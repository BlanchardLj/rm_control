/*******************************************************************************
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, Qiayuan Liao
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/

//
// Created by bruce on 2021/7/28.
//

#pragma once

#include <ros/ros.h>
#include <rm_msgs/ChassisCmd.h>
#include <rm_referee/referee/protocol.h>

namespace rm_common
{
class PowerLimit
{
public:
  PowerLimit(ros::NodeHandle& nh, const rm_msgs::ChassisCmd& chassis_cmd) : chassis_cmd_(chassis_cmd)
  {
    if (!nh.getParam("safety_power", safety_power_))
      ROS_ERROR("Safety power no defined (namespace: %s)", nh.getNamespace().c_str());
    if (!nh.getParam("capacitor_threshold", capacitor_threshold_))
      ROS_ERROR("Capacitor threshold no defined (namespace: %s)", nh.getNamespace().c_str());
    if (!nh.getParam("charge_power", charge_power_))
      ROS_ERROR("Charge power no defined (namespace: %s)", nh.getNamespace().c_str());
    if (!nh.getParam("extra_power", extra_power_))
      ROS_ERROR("Extra power no defined (namespace: %s)", nh.getNamespace().c_str());
    if (!nh.getParam("burst_power", burst_power_))
      ROS_ERROR("Burst power no defined (namespace: %s)", nh.getNamespace().c_str());
  }
  typedef enum
  {
    TEST = 0,
    BURST = 1,
    NORMAL = 2,
    CHARGE = 3,
  } Mode;

  void updateState(uint8_t state)
  {
    state_ = state;
  }
  void updateGameStatus(rm_msgs::GameStatus data)
  {
    game_status_ = data;
  }
  void updateGameRobotStatus(rm_msgs::GameRobotStatus data)
  {
    game_robot_status_ = data;
  }
  void updateReferee(rm_msgs::Referee data)
  {
    referee_ = data;
  }
  void updateRobotId(int data)
  {
    robot_id_ = data;
  }
  void updateCapacityData(rm_msgs::CapacityData data)
  {
    capacity_data_ = data;
  }
  uint8_t getState()
  {
    return state_;
  }
  double getLimitPower()
  {
    if (robot_id_ == rm_referee::RobotId::BLUE_SENTRY || robot_id_ == rm_referee::RobotId::RED_SENTRY)
      limit_power_ = 30;
    else if (robot_id_ == rm_referee::RobotId::RED_ENGINEER || robot_id_ == rm_referee::RobotId::BLUE_ENGINEER)
      limit_power_ = 400;
    else
    {  // standard and hero
      if (referee_.is_online)
      {
        if (capacity_data_.is_online)
        {
          if (game_status_.game_progress == 1)
            return 30;  // calibra
          if (game_robot_status_.chassis_power_limit > 120)
            limit_power_ = burst_power_;
          else
          {
            switch (state_)
            {
              case TEST:
                test();
                break;
              case BURST:
                burst();
                break;
              case NORMAL:
                normal();
                break;
              case CHARGE:
                charge();
                break;
            }
            if (state_ != Mode::BURST &&
                (abs(capacity_data_.limit_power - game_robot_status_.chassis_power_limit) < 0.05))
              normal();
          }
        }
        else
          limit_power_ = game_robot_status_.chassis_power_limit;
      }
      else
        limit_power_ = safety_power_;
    }
    return limit_power_;
  }

private:
  void charge()
  {
    limit_power_ = game_robot_status_.chassis_power_limit * 0.85;
  }
  void normal()
  {
    limit_power_ = game_robot_status_.chassis_power_limit;
  }
  void test()
  {
    limit_power_ = 0.0;
  }
  void burst()
  {
    if (capacity_data_.cap_power > capacitor_threshold_)
    {
      if (chassis_cmd_.mode == rm_msgs::ChassisCmd::GYRO)
        limit_power_ = game_robot_status_.chassis_power_limit + extra_power_;
      else
        limit_power_ = burst_power_;
    }
    else
      limit_power_ = game_robot_status_.chassis_power_limit;
  }

  int robot_id_;
  double limit_power_;
  double safety_power_{};
  double capacitor_threshold_{};
  double charge_power_{};
  double extra_power_{};
  double burst_power_{};
  uint8_t state_{};
  const rm_msgs::ChassisCmd& chassis_cmd_;
  rm_msgs::GameRobotStatus game_robot_status_;
  rm_msgs::GameStatus game_status_;
  rm_msgs::CapacityData capacity_data_;
  rm_msgs::Referee referee_;
};
}  // namespace rm_common
