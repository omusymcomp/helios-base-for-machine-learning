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

#ifndef BASE_OPTIONS_H
#define BASE_OPTIONS_H

#include <string>

namespace rcsc {
class CmdLineParser;
}

/*!
  \class Options
  \brief singleton command line option holder
*/
class Options {
public:

private:

    std::string M_parameter_dir; //!< the file path to write manual parameter files

    Options(); // private for singleton

    // not used
    Options( const Options & );
    const Options & operator=( const Options & );

public:

    static
    Options & instance();

    /*!
      \brief singleton instance interface
      \return const reference to local static instance
    */
    static
    const Options & i()
    {
      return instance();
    }

    bool init( rcsc::CmdLineParser & cmd_parser );

    const std::string & manualParameterDir() const { return M_parameter_dir; }

};

#endif
