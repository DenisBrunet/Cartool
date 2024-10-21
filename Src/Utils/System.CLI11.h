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

#include    "CLI/CLI.hpp"
#include    "Strings.Utils.h"

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
                                        // Showing the default after the type of option, already in "[]"
#define                 ShowDefault             default_str
                                        // Special check for a given option
#define                 CheckOption             check


//----------------------------------------------------------------------------
                                        // None of these definition allocate nor make use of any variable
inline CLI::Option*     DefineCLIFlag           ( CLI::App* app, const string name1, const string name2, const string& description )
{
return  app->add_flag ( CLIBuildOptionName ( name1, name2 ), description );
}

                                        // Defining options from scalar type
inline CLI::Option*     DefineCLIOptionString   ( CLI::App* app, const string name1, const string name2, const string& description )
{
return  app->add_option ( CLIBuildOptionName ( name1, name2 ), description )
        ->TypeOfOption          ( "TEXT" );
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


//----------------------------------------------------------------------------
                                        // Retrieving options
//----------------------------------------------------------------------------

inline bool             HasCLIFlag              ( CLI::App* app, const string option )
{
return  (bool) GetCLIOption ( app, option )->count ();
}

inline bool             HasCLIOption            ( CLI::App* app, const string option )
{
return  (bool) GetCLIOption ( app, option )->count ();
}


                                        // All missing options will return a default value according to their types: "" or 0
inline string           GetCLIOptionString      ( CLI::App* app, const string option )
{
return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->results ()[ 0 ] : string();
}

inline int              GetCLIOptionInt         ( CLI::App* app, const string option )
{
return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->as<int> () : 0;
}

inline double           GetCLIOptionDouble      ( CLI::App* app, const string option )
{
return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->as<double> () : 0;
}


                                        // Vectors versions
inline vector<string>   GetCLIOptionStrings     ( CLI::App* app, const string option )
{
return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->results () : vector<string>();
}

inline vector<int>      GetCLIOptionInts        ( CLI::App* app, const string option )
{
return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->as<vector<int>> () : vector<int>();
}

inline vector<double>   GetCLIOptionDoubles     ( CLI::App* app, const string option )
{
return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->as<vector<double>> () : vector<double>();
}


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

inline bool             IsSubCommandUsed        ( const CLI::App* subcommand )
{
return  subcommand && *subcommand;  // true if sub-command has been invoked - to be more thorough, we could count options has in IsGroupUsed
}


                                        // Retrieving group description
inline string           GetCLIGroupDescription  ( const CLI::Option_group* group )
{
return  group /*IsGroupUsed ( group )*/ ? group->get_description () : string();
}
                                        // Retrieving option description
inline string           GetCLIOptionDescription ( CLI::App* app, const string option )
{
return  HasCLIOption ( app, option ) ? GetCLIOption ( app, option )->get_description () : string();
}


}
