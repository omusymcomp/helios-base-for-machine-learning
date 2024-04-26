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

#include "bhv_basic_move.h"

#include "strategy.h"

#include "bhv_basic_tackle.h"
#include "neck_offensive_intercept_neck.h"

#include "basic_actions/basic_actions.h"
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/body_intercept.h"
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "basic_actions/neck_turn_to_low_conf_teammate.h"
#include "options.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include "neck_offensive_intercept_neck.h"

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicMove::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove" );

    //-----------------------------------------------
    // tackle
    if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
    {
        return true;
    }

    const WorldModel & wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

        /*--------------------------------------------------------*/
    //Gliders2d: pressing

    int pressing = 12;
    if ( readParameters() )
    {
        std::string pressing_parameter = "pressing";
        auto it =  M_param_map.find(pressing_parameter);
        if ( it != M_param_map.end() )
        {
            pressing = M_param_map["pressing"];
            dlog.addText( Logger::TEAM,
                        __FILE__": Pressing=%d",
                        pressing);
        }
    }

    // if ( role >= 6 && role <= 8 && wm.ball().pos().x > -30.0 && wm.self().pos().x < 10.0 )
    //     pressing = 7;


    // if (fabs(wm.ball().pos().y) > 22.0 && wm.ball().pos().x < 0.0 && wm.ball().pos().x > -36.5 && (role == 4 || role == 5) )
    //     pressing = 23;

    agent->debugClient().addMessage( "Pressing%d", pressing );


    if ( ! wm.kickableTeammate()
            && ( self_min <= 3
                || ( self_min <= mate_min
                    && self_min < opp_min + pressing )
                )
            )
    {
        dlog.addText( Logger::TEAM,
                        __FILE__": intercept" );
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );

        return true;
    }

    const double dash_power = Strategy::get_normal_dash_power( wm );

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f",
                  target_point.x, target_point.y,
                  dist_thr );

    agent->debugClient().addMessage( "BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );

    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power
                           ).execute( agent ) )
    {
        Body_TurnToBall().execute( agent );
    }

    if ( wm.kickableOpponent()
         && wm.ball().distFromSelf() < 18.0 )
    {
        agent->setNeckAction( new Neck_TurnToBall() );
    }
    else
    {
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */

bool
Bhv_BasicMove::readParameters( )
{   
    double value;
    char parameter_name[9];

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
        std::cerr <<  __FILE__ << ":"
                  << "(readParameters)"
                  << " ***ERROR*** could not open the file ["
                  << parameter_file << "]" << std::endl;
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
                             "%8s %lf",
                             parameter_name, &value) != 2 )
            {
                std::cerr << __FILE__ << ":"
                          << "(readparameter)"
                          << " ***ERROR*** could not read the line ["
                          << line_buf << "]" << std::endl;
                return false;
            }
            std::string str_parameter_name = parameter_name;
            M_param_map.insert({str_parameter_name, value});
        }
    }
    return true;
}