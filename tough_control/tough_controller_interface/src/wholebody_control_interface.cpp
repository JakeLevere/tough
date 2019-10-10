#include "tough_controller_interface/wholebody_control_interface.h"

WholebodyControlInterface::WholebodyControlInterface(ros::NodeHandle &nh)
    : ToughControlInterface(nh), chestController_(nh), armController_(nh)
{
  m_wholebodyPub = nh_.advertise<ihmc_msgs::WholeBodyTrajectoryRosMessage>(
      control_topic_prefix_ + TOUGH_COMMON_NAMES::WHOLEBODY_TRAJECTORY_TOPIC, 10, true);

  rd_->getLeftArmJointNames(left_arm_joint_names_);
  rd_->getRightArmJointNames(right_arm_joint_names_);
  rd_->getChestJointNames(chest_joint_names_);
}

bool WholebodyControlInterface::getJointSpaceState(std::vector<double> &joints, RobotSide side)
{
  joints.clear();
  state_informer_->getJointPositions(joints);

  return !joints.empty();
}

bool WholebodyControlInterface::getTaskSpaceState(geometry_msgs::Pose &pose, RobotSide side, std::string fixedFrame)
{
  return state_informer_->getCurrentPose(rd_->getPelvisFrame(), pose, fixedFrame);
}

void WholebodyControlInterface::executeTrajectory(const moveit_msgs::RobotTrajectory &traj)
{
  return executeTrajectory(traj.joint_trajectory);
}

void WholebodyControlInterface::executeTrajectory(const trajectory_msgs::JointTrajectory &traj)
{
  ihmc_msgs::WholeBodyTrajectoryRosMessage wholeBodyMsg;

  initializeWholebodyMessage(wholeBodyMsg);
  parseTrajectory(traj, wholeBodyMsg);
  m_wholebodyPub.publish(wholeBodyMsg);
  ros::Duration(0.1).sleep();
}

void WholebodyControlInterface::initializeWholebodyMessage(ihmc_msgs::WholeBodyTrajectoryRosMessage &wholeBodyMsg)
{
  // Setting unique id non zero for messages to be used
  wholeBodyMsg.unique_id = ros::Time::now().toSec();

  // setting default values for empty messages
  wholeBodyMsg.left_arm_trajectory_message.robot_side = LEFT;
  wholeBodyMsg.right_arm_trajectory_message.robot_side = RIGHT;

  wholeBodyMsg.left_foot_trajectory_message.robot_side = LEFT;
  wholeBodyMsg.right_foot_trajectory_message.robot_side = RIGHT;

  wholeBodyMsg.left_hand_trajectory_message.robot_side = LEFT;
  wholeBodyMsg.right_hand_trajectory_message.robot_side = RIGHT;

  // Specifying execution modes
  wholeBodyMsg.chest_trajectory_message.execution_mode = ihmc_msgs::ChestTrajectoryRosMessage::OVERRIDE;
  wholeBodyMsg.right_arm_trajectory_message.execution_mode = ihmc_msgs::ArmTrajectoryRosMessage::OVERRIDE;
  wholeBodyMsg.left_arm_trajectory_message.execution_mode = ihmc_msgs::ArmTrajectoryRosMessage::OVERRIDE;
  wholeBodyMsg.left_foot_trajectory_message.execution_mode = ihmc_msgs::FootTrajectoryRosMessage::OVERRIDE;
  wholeBodyMsg.right_foot_trajectory_message.execution_mode = ihmc_msgs::FootTrajectoryRosMessage::OVERRIDE;
  wholeBodyMsg.left_hand_trajectory_message.execution_mode = ihmc_msgs::HandTrajectoryRosMessage::OVERRIDE;
  wholeBodyMsg.right_hand_trajectory_message.execution_mode = ihmc_msgs::HandTrajectoryRosMessage::OVERRIDE;

  // unique ID will be updated if there is a valid trajectory point available
  wholeBodyMsg.chest_trajectory_message.unique_id = 0;
  wholeBodyMsg.right_arm_trajectory_message.unique_id = 0;
  wholeBodyMsg.left_arm_trajectory_message.unique_id = 0;

  // taskspace trajectories
  wholeBodyMsg.left_foot_trajectory_message.unique_id = 0;
  wholeBodyMsg.right_foot_trajectory_message.unique_id = 0;
  wholeBodyMsg.left_hand_trajectory_message.unique_id = 0;
  wholeBodyMsg.right_hand_trajectory_message.unique_id = 0;

  // not supported in 0.8.2
  // ihmc_msgs::FrameInformationRosMessage frameInfo;
  // frameInfo.data_reference_frame_id = rd_->getPelvisZUPFrameHash();
  // frameInfo.trajectory_reference_frame_id = rd_->getPelvisZUPFrameHash();

  // wholeBodyMsg.chest_trajectory_message.frame_information = frameInfo;
  // wholeBodyMsg.left_foot_trajectory_message.frame_information = frameInfo;
  // wholeBodyMsg.right_foot_trajectory_message.frame_information = frameInfo;
  // wholeBodyMsg.pelvis_trajectory_message.frame_information = frameInfo;
  // wholeBodyMsg.left_hand_trajectory_message.frame_information = frameInfo;
  // wholeBodyMsg.right_hand_trajectory_message.frame_information = frameInfo;
}

void WholebodyControlInterface::parseTrajectory(const trajectory_msgs::JointTrajectory &traj,
                                                ihmc_msgs::WholeBodyTrajectoryRosMessage &wholeBodyMsg)
{
  long chest_start = -1, l_arm_start = -1, r_arm_start = -1;

  auto it = std::find(traj.joint_names.begin(), traj.joint_names.end(), chest_joint_names_.at(0));
  if (it == traj.joint_names.end())
  {
    // does not contain chest points
  }
  else
  {
    // set the chest indices
    chest_start = std::distance(traj.joint_names.begin(), it);
    if (!validateJointSequenceInTrajectory(traj.joint_names, chest_joint_names_, chest_start))
    {
      return;
    }
    chestController_.setupFrameAndMode(wholeBodyMsg.chest_trajectory_message);
  }

  it = std::find(traj.joint_names.begin(), traj.joint_names.end(), left_arm_joint_names_.at(0));
  if (it == traj.joint_names.end())
  {
    // does not contain left arm points
  }
  else
  {
    // set the left arm indices
    l_arm_start = std::distance(traj.joint_names.begin(), it);
    if (!validateJointSequenceInTrajectory(traj.joint_names, left_arm_joint_names_, l_arm_start))
    {
      return;
    }
    armController_.setupArmMessage(RobotSide::LEFT, wholeBodyMsg.left_arm_trajectory_message);
  }

  it = std::find(traj.joint_names.begin(), traj.joint_names.end(), right_arm_joint_names_.at(0));
  if (it == traj.joint_names.end())
  {
    // does not contain right arm points
  }
  else
  {
    // set the right arm indices
    r_arm_start = std::distance(traj.joint_names.begin(), it);
    if (!validateJointSequenceInTrajectory(traj.joint_names, right_arm_joint_names_, r_arm_start))
    {
      return;
    }
    armController_.setupArmMessage(RobotSide::RIGHT, wholeBodyMsg.right_arm_trajectory_message);
  }

  double traj_point_time = 0.0;
  for (size_t i = 0; i < traj.points.size(); i++)
  {
    traj_point_time = traj.points.at(i).time_from_start.toSec();

    // chest - move to a new function later
    if (chest_start >= 0)
    {
      geometry_msgs::Quaternion quat;
      createChestQuaternion(chest_start, traj.points.at(i), quat);
      chestController_.appendChestTrajectoryPoint(quat, wholeBodyMsg.chest_trajectory_message, traj_point_time);
    }
    // arms
    if (l_arm_start >= 0)
    {
      std::vector<double> positions;
      positions.assign(traj.points.at(i).positions.begin() + l_arm_start,
                       traj.points.at(i).positions.begin() + l_arm_start + left_arm_joint_names_.size());
      armController_.appendTrajectoryPoint(wholeBodyMsg.left_arm_trajectory_message, traj_point_time, positions);
    }

    if (r_arm_start >= 0)
    {
      std::vector<double> positions;
      positions.assign(traj.points.at(i).positions.begin() + r_arm_start,
                       traj.points.at(i).positions.begin() + r_arm_start + right_arm_joint_names_.size());
      armController_.appendTrajectoryPoint(wholeBodyMsg.right_arm_trajectory_message, traj_point_time, positions);
    }
  }
}

bool WholebodyControlInterface::validateJointSequenceInTrajectory(const std::vector<std::string> &traj_joint_names,
                                                                  const std::vector<std::string> &joint_names,
                                                                  long start)
{
  for (auto joint_name_it = joint_names.begin(); joint_name_it != joint_names.end(); ++joint_name_it, ++start)
  {
    if (traj_joint_names[start] != *joint_name_it)
    {
      // joints in trajectory are not in the expected sequence.
      ROS_ERROR("Joints in the trajectory are not in the expected sequence.");
      return false;
    }
  }

  return true;
}
