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
#include <config.h>
#endif

#include "read_parameters.h"

#include <rcsc/player/player_agent.h>

#include "options.h"

/*-------------------------------------------------------------------*/
/*!

 */
double
Read_Parameters::get_param( const std::string & param_name )
{
    if ( ! read() )
    {
        return 0;
    }

    double param = 0;
    if ( ! M_param_map.empty() )
    {
        auto it = M_param_map.find(param_name);
        if ( it != M_param_map.end() )
        {
            param = M_param_map[param_name];
        }
    }
    else
    {
        std::cerr << __FILE__ 
                    << ": (get_param)"
                    << " ***ERROR*** could not get manual parameter"
                    << std::endl;
        return 0;
    }        

    return param;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Read_Parameters::read()
{   
    double value;
    char parameter_name[20];

    std::string line_buf;

    std::string parameter_dir = Options::i().manualParameterDir();
    
    std::string parameter_file = parameter_dir + "parameters.txt";

    std::ifstream f( parameter_file.c_str() );

    // std::cerr << __FILE__ << ":"
    //           << "(readline_buf)"
    //           <<"*** ["
    //           << line_buf
    //           <<" ]" << std::endl;

    if ( ! f.is_open() )
    {
        // std::cerr <<  __FILE__ << ":"
        //           << "(readParameters)"
        //           << " ***ERROR*** could not open the file ["
        //           << parameter_file << "]" << std::endl;
        return false;
    }

    while ( std::getline( f, line_buf ))    
    {
        if( line_buf.empty() )
        {
            continue;
        }

        if ( ! std::isdigit(static_cast<unsigned char>(line_buf[0])) )
        {
            if( std::sscanf( line_buf.data(),
                             "%19s %lf",
                             parameter_name, &value) != 2 )
            {
                // std::cerr << __FILE__ << ":"
                //           << "(readparameter)"
                //           << " ***ERROR*** could not read the line ["
                //           << line_buf << "]" << std::endl;
                return false;
            }
            std::string str_parameter_name = parameter_name;
            M_param_map.insert({str_parameter_name, value});
        }
    }
    return true;
}