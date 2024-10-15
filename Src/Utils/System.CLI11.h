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

                                        // Short and Long Names are optionals, but at least one should be specified
inline string   CLIBuildOptionName ( const string& Name1, const string& Name2 )
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
                                        // Setting options and flags

                                        // Using a macro so we can define 2 variables: the one requested, and a CLI::Option* one
                                        // Later, use  HasOption, HasFlag, GetVariableDescription
#define         AddFlag(ToGroup,FlagVariable,ShortName,LongName,Description) \
bool            FlagVariable    = false; \
CLI::Option*    opt##FlagVariable       = ( ToGroup ? ToGroup : &app )->add_flag ( CLIBuildOptionName(ShortName,LongName), FlagVariable, Description )


#define         AddOptionInt(ToGroup,OptionVariable,ShortName,LongName,Description) \
int             OptionVariable  = 0; \
CLI::Option*    opt##OptionVariable     = ( ToGroup ? ToGroup : &app )->add_option ( CLIBuildOptionName(ShortName,LongName), OptionVariable, Description )

#define         AddOptionInts(ToGroup,OptionVariable,HowMany,ShortName,LongName,Description) \
vector<int>     OptionVariable; \
CLI::Option*    opt##OptionVariable     = ( ToGroup ? ToGroup : &app )->add_option ( CLIBuildOptionName(ShortName,LongName), OptionVariable, Description ) \
->delimiter     ( ',' ) \
->expected      ( HowMany )


#define         AddOptionDouble(ToGroup,OptionVariable,ShortName,LongName,Description) \
double          OptionVariable  = 0; \
CLI::Option*    opt##OptionVariable     = ( ToGroup ? ToGroup : &app )->add_option ( CLIBuildOptionName(ShortName,LongName), OptionVariable, Description )

#define         AddOptionDoubles(ToGroup,OptionVariable,HowMany,ShortName,LongName,Description) \
vector<double>  OptionVariable; \
CLI::Option*    opt##OptionVariable     = ( ToGroup ? ToGroup : &app )->add_option ( CLIBuildOptionName(ShortName,LongName), OptionVariable, Description ) \
->delimiter     ( ',' ) \
->expected      ( HowMany )


#define         AddOptionString(ToGroup,OptionVariable,ShortName,LongName,Description) \
string          OptionVariable; \
CLI::Option*    opt##OptionVariable     = ( ToGroup ? ToGroup : &app )->add_option ( CLIBuildOptionName(ShortName,LongName), OptionVariable, Description )

#define         AddOptionStrings(ToGroup,OptionVariable,HowMany,ShortName,LongName,Description) \
vector<string>  OptionVariable; \
CLI::Option*    opt##OptionVariable     = ( ToGroup ? ToGroup : &app )->add_option ( CLIBuildOptionName(ShortName,LongName), OptionVariable, Description ) \
->expected      ( HowMany )

                                        // Giving some semantic to these options
#define         AnyNumberOfOptions                  require_option ( ::crtl::Lowest<int> () )
#define         ExclusiveOptions                    require_option ( 1 )
#define         NoMoreOptions(MAXO)                 require_option ( 0, MAXO )
#define         NumberOfOptions(MINO,MAXO)          require_option ( MINO, MAXO )


//----------------------------------------------------------------------------
                                        // Retrieving options and flags

                                        // Testing if a flag/option has been specified
#define         HasOption(Variable)                 ((bool) (opt##Variable)->count ())
#define         HasFlag(Variable)                   HasOption(Variable)
                                        // As a proper function, with string parameters
inline bool     HasOptionFct ( const CLI::App& app, const char* subcommandname, const char* optionname )    
{ 
return  StringIsEmpty ( subcommandname ) ? (bool) app.get_option ( optionname )->count ()
                                         : (bool) app.get_subcommand ( subcommandname )->get_option ( optionname )->count (); 
}

                                        // Testing if a sub-command flag/option has been specified
#define         HasSubCommand(Sub,Subcommand)       ((Sub)->get_subcommand ( Subcommand ) )
#define         HasSubOption(Sub,Option)            ((Sub)->get_option ( Option )->count ())
#define         HasSubFlag(Sub,Option)              HasSubOption(Sub,Option)
#define         GetSubOptionInt(Sub,Option)         (HasSubOption(Sub,Option) ? (Sub)->get_option ( Option )->as<int> ()            : 0 )
#define         GetSubOptionInts(Sub,Option)        (HasSubOption(Sub,Option) ? (Sub)->get_option ( Option )->as<vector<int>> ()    : vector<int>() )
#define         GetSubOptionDouble(Sub,Option)      (HasSubOption(Sub,Option) ? (Sub)->get_option ( Option )->as<double> ()         : 0 )
#define         GetSubOptionDoubles(Sub,Option)     (HasSubOption(Sub,Option) ? (Sub)->get_option ( Option )->as<vector<double>> () : vector<double>() )
#define         GetSubOptionString(Sub,Option)      (HasSubOption(Sub,Option) ? (Sub)->get_option ( Option )->results ()[ 0 ]       : "" )
#define         GetSubOptionStrings(Sub,Option)     (HasSubOption(Sub,Option) ? (Sub)->get_option ( Option )->results ()            : vector<string>() )

                                        // Is a given Group or Sub-command being used?
inline bool     IsGroupUsed         ( const CLI::Option_group* group )
{
if ( group == 0 )
    return  false;

for ( const auto* opt : group->get_options () )
    if ( opt->count() > 0)
        return  true;

return  false;
}
inline bool     IsSubCommandUsed    ( const CLI::App* subcommand )
{
return  subcommand && *subcommand;  // true if sub-command has been invoked - to be more thorough, we could count options has in IsGroupUsed
}

                                        // Retrieving option description
#define         GetVariableDescription(Variable)    (opt##Variable)->get_description ()
#define         GetGroupDescription(Group)          (Group)->get_description ()

}
