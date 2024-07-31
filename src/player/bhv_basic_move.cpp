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
#include "bhv_marlik_block.h"
#include "read_parameters.h"

#include "basic_actions/basic_actions.h"
#include "basic_actions/body_go_to_point.h"
#include "basic_actions/body_intercept.h"
#include "basic_actions/neck_turn_to_ball_or_scan.h"
#include "basic_actions/neck_turn_to_low_conf_teammate.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/soccer_intention.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicMove::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove" );

    //-----------------------------------------------
    // tackle
    // EP: tackle probability
    double tackle_prob = 0.0;
    if ( wm.ball().pos().x < 0.0 )
    {
        tackle_prob = Read_Parameters().get_param( "tackle_prob_our_side" );
        if ( tackle_prob == std::numeric_limits<double>::max() )
            tackle_prob = 0.5;
        else
            tackle_prob = 1.0 / ( 1.0 + exp( tackle_prob ) );
    }
    else
    {
        tackle_prob = Read_Parameters().get_param( "tackle_prob_opp_side" );
        if ( tackle_prob == std::numeric_limits<double>::max() )
            tackle_prob = 0.8;
        else
            tackle_prob = 1.0 / ( 1.0 + exp( tackle_prob ) );
    }

    if ( Bhv_BasicTackle( tackle_prob, 80.0 ).execute( agent ) )
    {
        return true;
    }

    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    const Vector2D target_point = Strategy::i().getPosition( wm.self().unum() );

        /*--------------------------------------------------------*/
    //EP: role number
    const int role_unum = Strategy::i().roleNumber( wm.self().unum() );
    std::string role_name = "";
    if ( role_unum == 2 || role_unum == 3 )
    {
        role_name = "CenterBack";
    }
    else if ( role_unum == 4 || role_unum == 5 )
    {
        role_name = "SideBack";
    }
    else if ( role_unum >= 6 && role_unum <= 8 )
    {
        role_name = "MidFielder";
    }

    //EP: block
    double block = Read_Parameters().get_param( "block" );
    if ( block == std::numeric_limits<double>::max() )
        block = -10.0;

    const Vector2D ball_pos = wm.ball().pos();
    const Vector2D self_pos = wm.self().pos();
    
    if (ball_pos.x < block)
    {
        double block_line = -38.0;

        // acknowledgement: fragments of Marlik-2012 code
        if ( role_name == "CenterBack"
             && target_point.x < block_line 
             && ( ball_pos.absY() < 18.0 && ball_pos.absY() > 6.0 )
             && ( opp_min <= 3 && opp_min <= mate_min && ball_pos.dist( self_pos ) < 9.5) )
        {
            // do noting
        }
        else if ( role_name == "CenterBack" 
                  && ball_pos.absY() < 22.0 )
        {
            // do noting
        }
        else if( Bhv_MarlikBlock().execute( agent ) )
        {
            agent->debugClient().addMessage( "MarlikBlock" );
            return true;
        }
    } // end of block

    //EP: pressing
    double pressing = 0;
    pressing = Read_Parameters().get_param( "pressing" );
    if ( pressing == std::numeric_limits<double>::max() )
        pressing = 12;

    if ( role_name == "MidFielder"
         && wm.ball().pos().x > -30.0 
         && wm.self().pos().x < 10.0 )
    {
        pressing = Read_Parameters().get_param( "pressing_mid" );
        if ( pressing == std::numeric_limits<double>::max() )
            pressing = 7;
    }

    if ( role_name == "SideBack"
         && wm.ball().pos().absY() > 22.0 
         && (wm.ball().pos().x < 0.0 && wm.ball().pos().x > -36.5 ) )
    {
        pressing = Read_Parameters().get_param( "pressing_sideback" );
        if ( pressing == std::numeric_limits<double>::max() )
            pressing = 23;
    }

    if ( ! wm.kickableTeammate()
            && ( self_min <= 3
                || ( self_min <= mate_min
                    && self_min < opp_min + pressing )
                )
            )
    {
        dlog.addText( Logger::TEAM,
                        __FILE__": intercept" );
        dlog.addText( Logger::TEAM,
                        __FILE__": Pressing=%lf",
                        pressing);
        agent->debugClient().addMessage( "Pressing%lf", pressing );
        Body_Intercept().execute( agent );
        agent->setNeckAction( new Neck_OffensiveInterceptNeck() );

        return true;
    }

    //EP: offside trap
    double nearest_teammate_pos_x = 0.0;
    double second_near_teammate_pos_x = 0.0;

    const PlayerObject::Cont::const_iterator t_end = wm.teammatesFromSelf().end();
    for ( PlayerObject::Cont::const_iterator it = wm.teammatesFromSelf().begin();
          it != t_end;
          ++it )
    {
        double x = (*it)->pos().x;
        if ( x < second_near_teammate_pos_x )
        {
            second_near_teammate_pos_x = x;
            if ( second_near_teammate_pos_x < nearest_teammate_pos_x )
            {
                std::swap( nearest_teammate_pos_x, second_near_teammate_pos_x );
            }
        }
    }

    double offside_trap_thr1 = Read_Parameters().get_param( "offside_trap_thr1" );
    if ( pressing == std::numeric_limits<double>::max() )
        offside_trap_thr1 = 3.5;

    double offside_trap_thr2 = Read_Parameters().get_param( "offside_trap_thr2" );
    if ( pressing == std::numeric_limits<double>::max() )
        offside_trap_thr2 = 4.5;

    if ( self_pos.x < -37.0 && opp_min < mate_min
         && ( target_point.x > -37.5 || wm.ball().inertiaPoint( opp_min ).x > -36.5 )
         && second_near_teammate_pos_x + offside_trap_thr2 > self_pos.x
         && ball_pos.x - offside_trap_thr1 > self_pos.x )
    {
        Body_GoToPoint( Vector2D( self_pos.x + 15.0, self_pos.y ),
                        0.5,                                   // distance threshold to the target point
                        ServerParam::i().maxDashPower(),      // maximum dash power
                        ServerParam::i().maxDashPower(),     // preferred dash speed
                        2,                                  // preferred reach cycle
                        true,                              // save recovery
                        5.0 ).execute( agent );           // minimum turn buffer

       if( wm.maybeKickableOpponent()
           && wm.ball().distFromSelf() < 12.0 )
           agent->setNeckAction( new Neck_TurnToBall() );
       else
           agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
        agent->debugClient().addMessage( "OffSideTrap" );
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