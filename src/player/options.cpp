// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "options.h"

#include <rcsc/param/param_map.h>
#include <rcsc/param/cmd_line_parser.h>

#include <iostream>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

*/
Options &
Options::instance()
{
    static Options s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

*/
Options::Options()
    : M_parameter_dir( "./player/data/")
{

}

/*-------------------------------------------------------------------*/
/*!

*/
bool
Options::init( CmdLineParser & cmd_parser )
{
    ParamMap param_map( "helios-base common options" );

    param_map.add()
        ( "parameter-dir", "", &M_parameter_dir, "the directory where manual parameter exist." )
        ;

    // read values from the command line options
    cmd_parser.parse( param_map );

    if ( cmd_parser.count( "help" ) > 0 )
    {
        param_map.printHelp( std::cout );
        return false;
    }

    return true;
}
