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

#include    "CLI/CLI.hpp"
#include    "Strings.Utils.h"
#include    "Strings.Grep.h"
#include    "Files.TGoF.h"

using namespace std;

namespace crtl {
                                        // CLI11 Command-Line Interface utilities

//----------------------------------------------------------------------------
                                        // Setting options and flags
//----------------------------------------------------------------------------
                                        // string to Option*
inline CLI::Option*     GetCLIOption            ( CLI::App* app, const string option )
{
return  app->get_option ( option );
}

                                        // From a given option, exclude a list of other options
inline CLI::Option*     ExcludeCLIOptions       ( CLI::App* app, const char* option, const char* excluded1, const char* excluded2 = 0, const char* excluded3 = 0 )
{
CLI::Option*        opt         = GetCLIOption ( app, option );

if ( StringIsNotEmpty ( excluded1 ) )   opt->excludes ( GetCLIOption ( app, excluded1 ) );
if ( StringIsNotEmpty ( excluded2 ) )   opt->excludes ( GetCLIOption ( app, excluded2 ) );
if ( StringIsNotEmpty ( excluded3 ) )   opt->excludes ( GetCLIOption ( app, excluded3 ) );

return  opt;
}

                                        // Wrapper to needs method
inline CLI::Option*     NeedsCLIOption          ( CLI::App* app, const string needy, const string needed )
{
return  GetCLIOption ( app, needy )->needs ( GetCLIOption ( app, needed ) );
}

                                        // Short and Long Names are optionals, but at least one should be specified
inline string           CLIBuildOptionName      ( const string& Name1, const string& Name2 )
{
string          name        = Name1;

if ( ! Name2.empty () ) {

    if ( ! name.empty () )
        name   += ",";

    name   += Name2;
    }

return  name;
};


//----------------------------------------------------------------------------
                                        // Defining options
//----------------------------------------------------------------------------
                                        // Giving some semantic to these options (convert them to functions / methods at some points maybe?)
#define                 ExclusiveOptions        require_option ( 1 )

#define                 ZeroOrOneArgument       expected ( 0, 1 )
                                        // option type displayed after name of options, which allows some refinement beyond INTEGER, FLOAT etc...
#define                 TypeOfOption            type_name
                                        
                                        // Setting a default string - Help string will surrond string with "[]"
//constexpr char*       DefaultCLIPrefix    =   "DEFAULT:";     // adding a prefix looks nice, but it still messes up with default values
//#define               DefaultString(S)        default_str ( DefaultCLIPrefix + string ( S ) )
#define                 DefaultString(S)        default_str ( S )
                                        // Default values boil down to strings eventually
#define                 DefaultInteger(I)       DefaultString ( (const char*) IntegerToString ( I ) )
#define                 DefaultDouble(F)        DefaultString ( (const char*) FloatToString   ( F ) )

                                        // Special check for a given option
#define                 CheckOption             check
                                        // Side-effect is that the --help will also require any argument listed
#define                 Required                required

using                   interval            = pair<double,double>;


//----------------------------------------------------------------------------
                                        // None of these definition allocate nor make use of any variable
inline CLI::Option*     DefineCLIFlag           ( CLI::App* app, const string name1, const string name2, const string& description )
{
return  app->add_flag   ( CLIBuildOptionName ( name1, name2 ), description );
}

                                        // Defining options from scalar type
inline CLI::Option*     DefineCLIOptionString   ( CLI::App* app, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "TEXT" );
}


inline CLI::Option*     DefineCLIOptionEnum     ( CLI::App* app, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "ENUM" );
}


inline CLI::Option*     DefineCLIOptionFile     ( CLI::App* app, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "FILE" );
}


inline CLI::Option*     DefineCLIOptionInt      ( CLI::App* app, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "INTEGER" );
}


inline CLI::Option*     DefineCLIOptionDouble   ( CLI::App* app, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "FLOAT" );
}


inline CLI::Option*     DefineCLIOptionInterval ( CLI::App* app, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "INTERVAL" )
        ->CheckOption           ( []( const string& str ) { return ! StringGrep ( str.c_str (), GrepOneInterval, GrepOptionDefault ) ? "not a properly formed interval, as in 12.3-45.6" : ""; } );
}


//----------------------------------------------------------------------------
                                        // Defining options from vectors type
inline string                   AsManyString ( int howmany )  { return howmany == -1 ? "..." : ""; }
inline  CLI::MultiOptionPolicy  AsManyPolicy ( int howmany )  { return howmany == -1 ? CLI::MultiOptionPolicy::TakeAll : CLI::MultiOptionPolicy::Throw; }


inline CLI::Option*     DefineCLIOptionStrings  ( CLI::App* app, int howmany, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "TEXTS" + AsManyString ( howmany ) )
        ->delimiter             ( ',' )
        ->expected              ( howmany )
        ->multi_option_policy   ( AsManyPolicy ( howmany ) );
}


inline CLI::Option*     DefineCLIOptionEnums    ( CLI::App* app, int howmany, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "ENUMS" + AsManyString ( howmany ) )
        ->delimiter             ( ',' )
        ->expected              ( howmany )
        ->multi_option_policy   ( AsManyPolicy ( howmany ) );
}


inline CLI::Option*     DefineCLIOptionFiles    ( CLI::App* app, int howmany, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "FILES" + AsManyString ( howmany ) )
        ->delimiter             ( ',' )
        ->expected              ( howmany )
        ->multi_option_policy   ( AsManyPolicy ( howmany ) );
}


inline CLI::Option*     DefineCLIOptionInts     ( CLI::App* app, int howmany, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "INTEGERS" + AsManyString ( howmany ) )
        ->delimiter             ( ',' )
        ->expected              ( howmany )
        ->multi_option_policy   ( AsManyPolicy ( howmany ) );
}


inline CLI::Option*     DefineCLIOptionDoubles  ( CLI::App* app, int howmany, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "FLOATS" + AsManyString ( howmany ) )
        ->delimiter             ( ',' )
        ->expected              ( howmany )
        ->multi_option_policy   ( AsManyPolicy ( howmany ) );
}


inline CLI::Option*     DefineCLIOptionIntervals( CLI::App* app, int howmany, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "INTERVALS" + AsManyString ( howmany ) )
        ->delimiter             ( ',' )
        ->expected              ( howmany )
        ->multi_option_policy   ( AsManyPolicy ( howmany ) )
        ->CheckOption           ( []( const string& str ) { return ! StringGrep ( str.c_str (), GrepManyIntervals, GrepOptionDefault ) ? "not a properly formed series of intervals, as in 12.3-45.6" : ""; } );
}


//----------------------------------------------------------------------------
                                        // Retrieving options
//----------------------------------------------------------------------------

inline bool             HasCLIFlag              ( CLI::App* app, const string option )  { return  (bool) GetCLIOption ( app, option )->count ();    }
inline bool             HasCLIOption            ( CLI::App* app, const string option )  { return  (bool) GetCLIOption ( app, option )->count ();    }
inline string           GetCLIDefault           ( CLI::App* app, const string option )  { return         GetCLIOption ( app, option )->get_default_str (); }
inline bool             HasCLIDefault           ( CLI::App* app, const string option )  { return  ! GetCLIDefault ( app, option ).empty ();         }   // this is how CLI11 knows if a default has been provided


//----------------------------------------------------------------------------
                                        // If an option is missing, it will try to return the default value if it exists, otherwise it returns "" or 0 according to its type
inline string           GetCLIOptionString      ( CLI::App* app, const string option )
{
return  HasCLIOption  ( app, option )   ? GetCLIOption  ( app, option )->results ()[ 0 ]    // what user specified
      : HasCLIDefault ( app, option )   ? GetCLIDefault ( app, option )                     // or default if it exists
      :                                   string();                                         // or "" otherwise
/*
if      ( HasCLIOption  ( app, option ) )    

    return  GetCLIOption ( app, option )->results ()[ 0 ];

else if ( HasCLIDefault ( app, option ) ) {
                                        // default has been provided, but possibly with some string before
    char            buff[ KiloByte ];

    StringCopy      ( buff, GetCLIDefault ( app, option ).c_str () );
    StringDelete    ( buff, DefaultCLIPrefix );
    
    return  string ( buff );
    }

else 
    
    return  string ();
*/
}

inline string           GetCLIOptionEnum        ( CLI::App* app, const string option )  { return                    GetCLIOptionString ( app, option );             }
inline int              GetCLIOptionInt         ( CLI::App* app, const string option )  { return  StringToInteger ( GetCLIOptionString ( app, option ).c_str () );  }   // empty string / option will convert to 0
inline double           GetCLIOptionDouble      ( CLI::App* app, const string option )  { return  StringToDouble  ( GetCLIOptionString ( app, option ).c_str () );  }   // empty string / option will convert to 0


inline interval         GetCLIOptionInterval    ( CLI::App* app, const string option )  
{
interval            i;
sscanf ( GetCLIOptionString ( app, option ).c_str (), "%lf-%lf", &i.first, &i.second ); // simplest way
return  i;
}

                                        // Returns a resolved path directory string, if option exists, or an empty string otherwise
inline TFileName        GetCLIOptionDir    ( CLI::App* app, const string optiondir )
{
if ( optiondir.empty () || ! HasCLIOption ( app, optiondir ) )

    return  TFileName ( "" );
else                                    // resolve path
    return  TFileName ( GetCLIOptionString ( app, optiondir ).c_str (), TFilenameFlags ( TFilenameCorrectCase | TFilenameAbsolutePath | TFilenameExtendedPath ) );
}

                                        // Returns a resolved path file string, optionally using a directory option
inline TFileName        GetCLIOptionFile        ( CLI::App* app, const string option, const string optiondir = "" )  
{
TFileName           dir             = GetCLIOptionDir       ( app, optiondir );             // could be empty
TFileName           filename        = GetCLIOptionString    ( app, option    ).c_str ();    // not resolved path yet
                                        // compose with optional directory if path appears to be relative
if ( filename.IsRelativePath () && dir.IsNotEmpty () )
    filename    = dir + "\\" + filename;
                                        // resolve path
filename.CheckFileName ( TFilenameFlags ( TFilenameCorrectCase | TFilenameAbsolutePath | TFilenameExtendedPath ) );

return  filename;
}


//----------------------------------------------------------------------------
                                        // Vectors versions
                                        // Default values are not currently allowed
inline vector<string>   GetCLIOptionStrings     ( CLI::App* app, const string option )  { return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->results ()            : vector<string>(); }
inline vector<string>   GetCLIOptionEnums       ( CLI::App* app, const string option )  { return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->results ()            : vector<string>(); }
inline vector<int>      GetCLIOptionInts        ( CLI::App* app, const string option )  { return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->as<vector<int>> ()    : vector<int>   (); }
inline vector<double>   GetCLIOptionDoubles     ( CLI::App* app, const string option )  { return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->as<vector<double>> () : vector<double>(); }


inline vector<interval> GetCLIOptionIntervals   ( CLI::App* app, const string option )
{
if ( ! HasCLIOption ( app, option ) )
    return vector<interval>();

vector<string>      vs          = GetCLIOption ( app, option )->results ();
vector<interval>    vi;
interval            i;

for ( const auto& s : vs ) {
    sscanf ( s.c_str (), "%lf-%lf", &i.first, &i.second ); // simplest way
    vi.push_back ( i );
    }

return  vi;
}

                                        // Returns a resolved path list of files, optionally using a directory option
inline TGoF             GetCLIOptionFiles       ( CLI::App* app, const string option, const string optiondir = "" )
{
TFileName           dir             = GetCLIOptionDir       ( app, optiondir ); // could be empty
TGoF                relfiles        = GetCLIOptionStrings   ( app, option );    // not resolved paths yet
TGoF                absfiles;
TFileName           filename;

for ( int i = 0; i < (int) relfiles; i++ ) {
                                        // copy AND update file name at the same time
    filename    = relfiles[ i ];
                                        // compose with optional directory if path appears to be relative
    if ( filename.IsRelativePath () && dir.IsNotEmpty () )
        filename    = dir + "\\" + filename;
                                        // resolve path
    filename.CheckFileName ( TFilenameFlags ( TFilenameCorrectCase | TFilenameAbsolutePath | TFilenameExtendedPath ) );

    absfiles.Add ( filename );
    }

return  absfiles;
}


//----------------------------------------------------------------------------
                                        // Is a given Group or Sub-command being used?
inline bool             IsGroupUsed             ( const CLI::Option_group* group )
{
if ( group == 0 )
    return  false;

for ( const auto* opt : group->get_options () )
    if ( opt->count() > 0)
        return  true;

return  false;
}

inline bool             IsSubCommandUsed        ( const CLI::App* subcommand )          { return  subcommand && *subcommand; }  // true if sub-command has been invoked - to be more thorough, we could count options has in IsGroupUsed


inline string           GetCLIGroupDescription  ( const CLI::Option_group* group )      { return  group /*IsGroupUsed ( group )*/ ? group->get_description () : string(); }
inline string           GetCLIOptionDescription ( CLI::App* app, const string option )  { return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->get_description () : string(); }


}
