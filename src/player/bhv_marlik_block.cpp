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

/*

   Z----------------------------------Z
   |    Fragments of the                              |
   |    file Created By: Amir Tavafi  |
   |                                  |
   |    Date Created:    2009/11/30,  |
   |                     1388/08/09   |
   |                                  |
   Z----------------------------------Z

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_marlik_block.h"

#include "strategy.h"
#include "bhv_basic_tackle.h"

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
Bhv_MarlikBlock::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const Vector2D self_pos = wm.self().pos();
    const Vector2D ball_pos = wm.ball().pos();

    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();
    
    const int role_unum = Strategy::i().roleNumber( wm.self().unum() );
    std::string role_name = "";
    if ( role_unum == 2 || role_unum == 3 )
    {
        role_name = "CenterBack";
    }

    if ( role_name == "CenterBack"
         && ball_pos.x < -46.0
         && ( ball_pos.absY() < 18.0 && ball_pos.absY() > 6.0 )
         && ( opp_min <= 3 && opp_min <= mate_min )
         && ball_pos.dist( self_pos ) < 9.0 )
         if ( doBlockMove( agent ) )
            return true;
    
    if ( wm.opponentsFromBall().empty() )
        return false;
    
    const Vector2D nearest_opp_pos = wm.opponentsFromBall().front()->pos();

    double max_opp_dist = 11;
    if ( ball_pos.x < -10 )
        max_opp_dist = 7;
    
    if ( doInterceptBall( agent ) )
        return true;
    
    const double tackle_prob = 0.9;

    if ( Bhv_BasicTackle( tackle_prob, 60.0 ).execute( agent ) )
        return true;
    
    if ( role_name == "CenterBack"
         && ball_pos.x < -30.0 && ball_pos.absY() > 16.0 )
        return false;
    if ( role_unum < 6
         && ( ball_pos.x < -30.0 && ball_pos.x > -40.0 )
         && wm.countTeammatesIn( Circle2D( Vector2D( nearest_opp_pos.x - 3.5, nearest_opp_pos.y ), 3.5 ), 5, false ) > 1 )
        return false;
    if ( role_name == "CenterBack"
         && ball_pos.x > wm.ourDefenseLineX() + max_opp_dist
         && ( ball_pos.x > -30.0 && ball_pos.x < 0.0 ) )
        return false;
    if ( role_unum == 6 && ball_pos.x > wm.ourDefenseLineX() + 23.0 )
        return false;

    if ( role_unum == 2
         && ( ball_pos.x < wm.ourDefenseLineX() + 7.0 && ball_pos.x > -38.0 )
         && ball_pos.absY() < 12.0
         && ( ball_pos.y < self_pos.y + 7.5 && ball_pos.y > self_pos.y - 3.0 )
         && opp_min < mate_min )
    {
        if ( doBlockMove( agent ) )
            return true;
    }
    if ( role_unum == 3
         && ball_pos.x < wm.ourDefenseLineX() + 7.0 && ball_pos.x > -38.0
         && ball_pos.absY() < 12.0
         && ( ball_pos.y > self_pos.y + 7.0 && ball_pos.y < self_pos.y + 3.0 )
         && opp_min < mate_min )
    {
        if ( doBlockMove( agent ) )
            return true;
    }
    if ( role_unum == 6
         && ball_pos.x < -40.0 && ball_pos.absY() < 6.0
         && self_pos.dist( ball_pos ) < 5.0
         && opp_min <= mate_min )
    {
        if ( doBlockMove( agent ) )
            return true;
    }
    if ( role_unum < 6
         && ball_pos.x < wm.ourDefenseLineX() + 5.0
         && ball_pos.dist( self_pos ) < 4.0
         && ball_pos.x > -38.0
         && opp_min < mate_min )
    {
        if ( doBlockMove( agent ) )
            return true;
    }
  if( ( ( role_unum > 5 && role_unum <= 8 && ball_pos.x < 25.0) 
        || ( ( role_unum <=5 && ball_pos.x < -30.0 ) 
        || ( ball_pos.x < wm.ourDefenseLineX() + max_opp_dist + 2.0 && ball_pos.x > -30.0 && ball_pos.x < 0.0) ) ) 
        && opp_min < mate_min )
    {
        if ( ball_pos.x < -36 && ball_pos.absY() < 7
             && self_min > opp_min && self_min <= mate_min )
        {
            if ( doBlockMove( agent ) )
                return true;
        }
        if ( self_min > opp_min && self_min < mate_min + 1 )
        {
            if ( doBlockMove( agent ) )
                return true;
        }
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_MarlikBlock::doInterceptBall( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const Vector2D ball_pos = wm.ball().pos();

    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    if ( self_min < opp_min && self_min < mate_min )
    {
        Body_Intercept( true, ball_pos ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBall() );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_MarlikBlock::doBlockMove( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const Vector2D self_pos = wm.self().pos();
    const Vector2D ball_pos = wm.ball().pos();

    const int self_min = wm.interceptTable().selfStep();
    const int mate_min = wm.interceptTable().teammateStep();
    const int opp_min = wm.interceptTable().opponentStep();

    const int role_unum = Strategy::i().roleNumber( wm.self().unum() );

    static bool isChaporating;

    if ( wm.opponentsFromBall().empty() )
        return false;
    
    const Vector2D block_pos = getBlockPoint( agent );

    const Vector2D goal_pos = Vector2D( -52.0, 0.0 );

    const double dash_power = ServerParam::i().maxDashPower();

    const Vector2D nearest_opp_pos = wm.opponentsFromBall().front()->pos();
    const Vector2D nearest_opp_vel = wm.opponentsFromBall().front()->vel();
    rcsc::Vector2D next_opp_pos = wm.opponentsFromBall().front()->pos() + wm.opponentsFromBall().front()->vel();

    if ( ball_pos.x < -36.0 && ball_pos.absY() > 12.0
         && self_pos.dist( goal_pos ) > next_opp_pos.dist( goal_pos ) + 5.0 )
        return false;
    
    double min_opp_dist = 3.5;
    if ( ball_pos.x < -36.0 && ball_pos.absY() < 12.0 )
        min_opp_dist = 4.0;
    
    int min_intercept_cycle = 2;
    if ( ball_pos.x < -36.0 && ball_pos.absY() > 12.0
         && self_pos.dist( goal_pos ) < next_opp_pos.dist( goal_pos ) )
        min_intercept_cycle = 3;
    else if ( ball_pos.x < -36.0 && ball_pos.absY() < 13.0 )
        min_intercept_cycle = 4;
    else if ( ball_pos.dist( goal_pos ) < 13 )
        min_intercept_cycle = 5;
    else if ( ball_pos.x < -36.0 
              && role_unum == 6 )
        min_intercept_cycle = 5;
    else if ( role_unum == 7 || role_unum == 8 )
        min_intercept_cycle = 7;

    Vector2D opp_target_point = Vector2D( -52.5, nearest_opp_pos.y * 0.95 );

    if ( role_unum == 7 || role_unum == 9 )
        opp_target_point = Vector2D( -36.0, -12.0 );
    else if ( role_unum == 8 || role_unum == 10 )
        opp_target_point = Vector2D( -36.0, 12.0 );
    
    if ( ball_pos.x < -34.0 && ball_pos.x > -40.0 )
        opp_target_point = Vector2D( -48.0, 0.0 );
    
    if( ball_pos.x < -30 
        && ball_pos.x > -40.0 
        && ball_pos.absY() < 15 )
    {
        if( role_unum == 2 )
            opp_target_point = Vector2D( -52.5, -4.0 );
        if( role_unum == 3 )
            opp_target_point = Vector2D( -52.5, 4.0 );
    }

    if ( ball_pos.dist( Vector2D( -52.5, 0.0 ) ) < 14 && ball_pos.absY() > 8 )
        opp_target_point = Vector2D( -51.0, 0.0 );
    
    if ( ball_pos.x < -40.0 )
        opp_target_point = Vector2D( -49.0, 0.0 );
    
    if ( ball_pos.x < -40.0 )
        opp_target_point = Vector2D( -50.0, 0.0 );
    
    Line2D target_line = rcsc::Line2D( opp_target_point, nearest_opp_pos + nearest_opp_vel );
    
    static Vector2D opp_static_pos = next_opp_pos;

    if ( self_min <= 1 )
    {
        isChaporating = false;
        Body_Intercept( true, ball_pos ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
        return true;
    }
    if ( self_min < 20
         && ( self_min < mate_min || ( self_min < mate_min + 2 && mate_min > 3 ) )
         && self_min <= opp_min )
    {
        isChaporating = false;
        Body_Intercept( true, ball_pos ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
        return true;
    }
    if ( ball_pos.x < -49.0
         && ( ball_pos.absY() < 14 && ball_pos.absY() > 7.0 )
         && self_min < 5 )
    {
        isChaporating = false;
        Body_Intercept( true, ball_pos ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
        return true;
    }
    if ( role_unum > 5
         && ball_pos.dist( goal_pos ) < 13.0
         && self_pos.dist( goal_pos ) < ball_pos.dist( goal_pos )
         && self_min < 5 )
    {
        isChaporating = false;
        Body_Intercept( true, ball_pos ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
        return true;
    }

    if ( self_min < mate_min
         && self_min <= opp_min
         && opp_min >= 2 )
    {
        isChaporating = false;
        Body_Intercept( true, ball_pos ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
        return true;
    }

    int time_block_point = 0.0;
    if ( self_pos.dist( block_pos ) < 1.7
         && target_line.dist( self_pos ) < 1.25
         && target_line.dist( opp_static_pos ) < 0.2
         && self_min <= min_intercept_cycle )
    {
        isChaporating = false;
        time_block_point = 0;
        Body_Intercept( true, ball_pos ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
        return true;
    }
    else if ( ( role_unum == 7 || role_unum == 8 )
              && self_pos.dist( block_pos ) < 2.0
              && target_line.dist( self_pos ) < 2.0
              && target_line.dist( opp_static_pos ) < 0.5
              && self_min <= min_intercept_cycle )
    {
        isChaporating = false;
        time_block_point = 0;
        Body_Intercept( true, ball_pos ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
        return true;
    }
    else if ( ball_pos.x < -36.0 && ball_pos.absY() > 14.0
              && self_pos.dist( goal_pos ) + 1.5 < next_opp_pos.dist( goal_pos )
              && target_line.dist( self_pos ) < 2.0
              && target_line.dist( opp_static_pos ) < 0.5
              && self_pos.dist( next_opp_pos ) < 2.0 && self_pos.dist( block_pos ) < 3.0
              && opp_min < 3 )
    {
        isChaporating = false;
        time_block_point = 0;
        Body_Intercept( true, ball_pos ).execute( agent );
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
        return true;
    }
    
    if ( ( self_pos.dist( block_pos ) < 3.5 || self_pos.dist( next_opp_pos ) < 2.5 )
            && self_pos.dist( next_opp_pos ) < min_opp_dist
            && ball_pos.dist( nearest_opp_pos ) < 1.2
            && self_pos.dist( goal_pos ) + 0.8 < next_opp_pos.dist( goal_pos )
            && target_line.dist( self_pos ) < 0.9
            && target_line.dist( opp_static_pos ) < 0.2 )
    {
        if ( ! Body_GoToPoint( next_opp_pos, 0.4, dash_power, 1, false, true ).execute( agent ) )
        {
            AngleDeg body_angle = agent->world().ball().angleFromSelf();
            if ( self_pos.y < 0.0 )
                body_angle -= 90.0;
            else
                body_angle += 90.0;
            Body_TurnToAngle( body_angle ).execute( agent );
        }
        isChaporating = false;
    }
    else if ( ( self_pos.dist( block_pos ) < 4.0 || self_pos.dist( next_opp_pos ) < 2.2 )
                && self_pos.dist( next_opp_pos ) < 4.0
                && ball_pos.dist( nearest_opp_pos ) < 1.2
                && self_pos.dist( goal_pos ) + 0.8 < next_opp_pos.dist( goal_pos )
                && wm.opponentsFromBall().front()->vel().r() < 0.1
                && target_line.dist( self_pos ) < 0.9 )
    {
        if ( ! Body_GoToPoint( next_opp_pos, 0.4, dash_power, 1, false, true ).execute( agent ) )
        {
            AngleDeg body_angle = agent->world().ball().angleFromSelf();
            if ( self_pos.y < 0.0 )
                body_angle -= 90.0;
            else
                body_angle += 90.0;
            Body_TurnToAngle( body_angle ).execute( agent );
        }
        isChaporating = false;
    }
    else if ( ( self_pos.dist( block_pos ) < 2.0 
                && self_pos.dist( goal_pos ) < next_opp_pos.dist( goal_pos )
                && ball_pos.dist( next_opp_pos ) < 1.3 )
                || isChaporating )
    {
        if ( self_pos.dist( goal_pos ) + 1 > next_opp_pos.dist( goal_pos ) )
            isChaporating = false;
        if ( self_pos.dist( goal_pos ) + 1 > block_pos.dist( goal_pos ) )
            isChaporating = false;
        if ( ball_pos.dist( nearest_opp_pos ) > 1.1 )
            isChaporating = false;
        if ( self_pos.dist( block_pos ) > 4.0 )
            isChaporating = false;
        if ( ball_pos.x < -37.0 && ball_pos.absY() < 10.0
             && self_pos.dist( block_pos ) < 3.0 )
            isChaporating = false;
        if ( next_opp_pos.dist( self_pos ) > 3.0 )
            isChaporating = false;
        
        double power = 100;

        if ( next_opp_pos.dist( opp_static_pos ) > 0.3 )
            power = 100;
        else if ( next_opp_pos.dist( opp_static_pos ) > 0.2 )
            power = 80;
        else
            power = 60;
        
        AngleDeg body_angle = ( next_opp_pos - opp_target_point ).dir();
        if ( self_pos.y < 0 )
            body_angle -= 90.0;
        else
            body_angle += 90.0;
        
        if ( std::fabs( wm.self().body().degree() - body_angle.degree() ) < 10 )
        {
            Line2D opp_line( next_opp_pos, opp_target_point );

            double self_on_opp_line_x = opp_line.getX( self_pos.y );
            double self_on_opp_line_y = opp_line.getY( self_pos.x );

            if ( ball_pos.x < -30 && ball_pos.absY() > 15 )
            {
                if ( self_pos.y > 0 )
                {
                    if ( wm.self().stamina() < 4000 )
                    {
                        if ( self_pos.x < self_on_opp_line_x )
                            agent->doDash( power );
                        else
                            agent->doDash( -power );
                    }
                }
                else
                {
                    if ( wm.self().stamina() > 4000 )
                    {
                        if ( self_pos.x < self_on_opp_line_x )
                            agent->doDash( power );
                        else
                            agent->doDash( -power );
                    }
                }
            }
            else
            {
                if ( self_pos.y > 0 )
                {
                    if ( wm.self().stamina() > 4000 )
                    {
                        if ( self_pos.y < self_on_opp_line_y )
                            agent->doDash( power );
                        else
                            agent->doDash( -power );
                    }
                }
                else
                {
                    if ( wm.self().stamina() > 4000 )
                    {
                        if ( self_pos.y > self_on_opp_line_y )
                            agent->doDash( power );
                        else
                            agent->doDash( -power );
                    }
                }
            }
            time_block_point++;
        }
        else
        {
            Body_TurnToAngle( body_angle ).execute( agent );
            time_block_point++;
        }

        isChaporating = false;
    }
    else if ( ! Body_GoToPoint( block_pos, 0.4, dash_power, 1, 100, true, 30.0 ).execute( agent ) )
    {
        AngleDeg body_angle = ( next_opp_pos - opp_target_point ).dir();

        if ( self_pos.y < 0 )
            body_angle -= 90.0;
        else
            body_angle += 90.0;
        
        Body_TurnToAngle( body_angle ).execute( agent );
        time_block_point++;
        isChaporating = false;
    }
    else
        time_block_point = 0;
    
    opp_static_pos = next_opp_pos;
    isChaporating = false;

    if ( wm.ball().distFromSelf() < 20.0
         && ( wm.maybeKickableOpponent() || opp_min ) )
        agent->setNeckAction( new Neck_TurnToBall() );
    else
        agent->setNeckAction( new Neck_TurnToBallOrScan( 0 ) );
    
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
Vector2D
Bhv_MarlikBlock::getBlockPoint( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const Vector2D self_pos = wm.self().pos();
    const Vector2D ball_pos = wm.ball().pos();
    Vector2D block_pos = ball_pos;

    const int opp_min = wm.interceptTable().opponentStep();

    const int role_unum = Strategy::i().roleNumber( wm.self().unum() );

    double opp_dribble_speed = 0.675;

    if ( ball_pos.x < -36.5 || self_pos.x < -36.5 )
    {
        if ( ball_pos.absY() < 20.0 || self_pos.absY() < 20.0 )
            opp_dribble_speed = 0.575;
    }

    Vector2D predict_ball_pos = ball_pos;
    if ( ! wm.maybeKickableOpponent() )
        predict_ball_pos = wm.ball().inertiaPoint( opp_min );

    const Vector2D nearest_opp_pos = wm.opponentsFromBall().front()->pos();

    Vector2D opp_target_point = Vector2D( -52.5, nearest_opp_pos.y * 0.95 );

    if ( role_unum == 7 || role_unum == 9 )
        opp_target_point = Vector2D( -36.0, -12.0 );
    else if ( role_unum == 8 || role_unum == 10 )
        opp_target_point = Vector2D( -36.0, 12.0 );
    
    if ( ball_pos.x < -34.0 && ball_pos.x > -40.0 )
        opp_target_point = Vector2D( -48.0, 0.0 );
    
    if( ball_pos.x < -30 
        && ball_pos.x > -40.0 
        && ball_pos.absY() < 15 )
    {
        if( role_unum == 2 )
            opp_target_point = Vector2D( -52.5, -4.0 );
        if( role_unum == 3 )
            opp_target_point = Vector2D( -52.5, 4.0 );
    }

    if ( ball_pos.dist( Vector2D( -52.5, 0.0 ) ) < 14 && ball_pos.absY() > 8 )
        opp_target_point = Vector2D( -51.0, 0.0 );
    
    if ( ball_pos.x < -40.0 )
        opp_target_point = Vector2D( -49.0, 0.0 );
    
    if ( ball_pos.x < -40.0 )
        opp_target_point = Vector2D( -50.0, 0.0 );
    
    Vector2D next_ball_pos = predict_ball_pos;
    next_ball_pos += Vector2D::polar2vector( opp_dribble_speed, ( opp_target_point - predict_ball_pos ).dir() );

    int opp_move_step;
    if ( wm.maybeKickableOpponent() )
        opp_move_step = 0;
    else
        opp_move_step = opp_min;

    while ( wm.self().playerType().cyclesToReachDistance( self_pos.dist( next_ball_pos ) ) > opp_move_step
            && opp_move_step < 50 )
    {
        if ( next_ball_pos.x > 36.0 
             && ( role_unum == 7 || role_unum == 9 ) )
            opp_target_point = Vector2D( -36.0, -12.0 );
        else if ( next_ball_pos.x > 30.0
                  && role_unum == 11 )
            opp_target_point = Vector2D( -30.0, 0.0 );
        else if( next_ball_pos.x < -34.0
                 && next_ball_pos.x > -40.0 )
            opp_target_point = Vector2D( -48.0, 0.0 );
        else if ( next_ball_pos.x < -40.0 )
            opp_target_point = Vector2D( -48.0, 0.0 );
        
        next_ball_pos += Vector2D::polar2vector( opp_dribble_speed, ( opp_target_point - predict_ball_pos ).dir() );
        opp_move_step++;
    }

    if ( opp_move_step >= 50 )
        block_pos = Vector2D( -48.0, ball_pos.y );
    else
        block_pos = next_ball_pos;
    
    if ( role_unum < 6 && block_pos.x > 0.0 )
        block_pos.x = 10.0;
    
    return block_pos;
}