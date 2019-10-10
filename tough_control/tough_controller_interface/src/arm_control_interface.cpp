#include <tough_controller_interface/arm_control_interface.h>
#include <stdlib.h>
#include <visualization_msgs/Marker.h>
#include <tf/tf.h>

// add default pose for both arms. the values of joints are different.
ArmControlInterface::ArmControlInterface(ros::NodeHandle nh)
    : ToughControlInterface(nh), ZERO_POSE{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}
{
  id_++;
  armTrajectoryPublisher = nh_.advertise<ihmc_msgs::ArmTrajectoryRosMessage>(
      control_topic_prefix_ + TOUGH_COMMON_NAMES::ARM_TRAJECTORY_TOPIC, 1, true);
  taskSpaceTrajectoryPublisher = nh_.advertise<ihmc_msgs::HandTrajectoryRosMessage>(
      control_topic_prefix_ + TOUGH_COMMON_NAMES::HAND_TRAJECTORY_TOPIC, 1, true);
  homePositionPublisher =
      nh_.advertise<ihmc_msgs::GoHomeRosMessage>(control_topic_prefix_ + TOUGH_COMMON_NAMES::GO_HOME_TOPIC, 1, true);
  markerPub_ = nh_.advertise<visualization_msgs::Marker>(TOUGH_COMMON_NAMES::MARKER_TOPIC, 1, true);

  rd_->getLeftArmJointLimits(joint_limits_left_);
  rd_->getRightArmJointLimits(joint_limits_right_);

  // reduce the joint limits by 1cm to avoid excceeding limits at higher precision of float
  for (size_t i = 0; i < joint_limits_left_.size(); i++)
  {
    joint_limits_left_[i] = {joint_limits_left_[i].first + 0.01, joint_limits_left_[i].second - 0.01};
    joint_limits_right_[i] = {joint_limits_right_[i].first + 0.01, joint_limits_right_[i].second - 0.01};
  }

  NUM_ARM_JOINTS = joint_limits_left_.size();
}

ArmControlInterface::~ArmControlInterface()
{
  armTrajectorySubscriber.shutdown();
}

// ************ Mesages using ArmTrajectoryRosMessage  ******************** //

/*
 *This message commands the controller to move an arm in jointspace to the desired joint angles while
 going through the specified trajectory points. A third order polynomial function is used to
 interpolate between trajectory points. The jointTrajectoryMessages can have different waypoint times
 and different number of waypoints. If a joint trajectory message is empty, the controller will hold
 the last desired joint position while executing the other joint trajectories. A message with a
 unique id equals to 0 will be interpreted as invalid and will not be processed by the controller.
 This rule does not apply to the fields of this message.
*/

/**
 * @brief ArmControlInterface::appendTrajectoryPoint will append a joint trajectory point
 * @param msg is the reference of the current msg where point is to be appended.
 * @param time is the time between last and current joint trajectory waypoint
 * @param pos is the joint position vector (size would be 7 if there are 7 joints in the arm)
 */
void ArmControlInterface::appendTrajectoryPoint(ihmc_msgs::ArmTrajectoryRosMessage &armMsg, const float time,
                                                const std::vector<double> &pos)
{
  std::vector<std::pair<double, double>> *joint_limits_;
  joint_limits_ = armMsg.robot_side == LEFT ? &joint_limits_left_ : &joint_limits_right_;
  int id = ArmControlInterface::id_;

  if (pos.size() != NUM_ARM_JOINTS)
  {
    ROS_WARN("Check number of trajectory points");
    return;
  }
  // checking if all the joints are within joint limits
  for (int i = 0; i < NUM_ARM_JOINTS; i++)
  {
    ihmc_msgs::TrajectoryPoint1DRosMessage p;

    // checking if the position is within joint limits
    float position = pos[i];
    position = pos[i] < joint_limits_->at(i).first ? joint_limits_->at(i).first : pos[i];
    position = pos[i] > joint_limits_->at(i).second ? joint_limits_->at(i).second : pos[i];

    p.time = time;
    p.position = position;
    p.velocity = 0;
    p.unique_id = id_++;

    armMsg.joint_trajectory_messages[i].trajectory_points.push_back(p);
    armMsg.joint_trajectory_messages[i].unique_id = id;
    // weight should be set to NAN so the optimizer can use its predefined weights.
    // armMsg.joint_trajectory_messages[i].weight = std::nan(""); // not supported in 0.8.2
  }

  return;
}

/**
 * @brief ArmControlInterface::moveToDefaultPose moves the arm to a predefined default pose.
 * @param side is the the side of the arm to move.
 */
void ArmControlInterface::moveToDefaultPose(RobotSide side, float time)
{
  ihmc_msgs::GoHomeRosMessage go_home;
  go_home.body_part = ihmc_msgs::GoHomeRosMessage::ARM;
  go_home.robot_side = (int)side;
  go_home.trajectory_time = time;
  go_home.unique_id = ArmControlInterface::id_++;

  homePositionPublisher.publish(go_home);
  ros::Duration(0.5).sleep();
}

/**
 * @brief ArmControlInterface::moveToZeroPose moves the arm to a predefined zero pose.
 * @param side is the the side of the arm to move.
 */
void ArmControlInterface::moveToZeroPose(const RobotSide side, const float time)
{
  ihmc_msgs::ArmTrajectoryRosMessage arm_traj;
  arm_traj.joint_trajectory_messages.clear();

  arm_traj.joint_trajectory_messages.resize(NUM_ARM_JOINTS);
  arm_traj.robot_side = side;
  arm_traj.unique_id = id_++;

  if (side == RobotSide::LEFT)
    appendTrajectoryPoint(arm_traj, time, ZERO_POSE);
  else
    appendTrajectoryPoint(arm_traj, time, ZERO_POSE);

  armTrajectoryPublisher.publish(arm_traj);
}

/**
 * @brief ArmControlInterface::moveArmJoints moves the joints. accepts complete joint trajectory at once.
 * @param side is the side of the arm to move.
 * @param arm_pose is the vector of vector of joint trajectories
 * @param time is the total time for the complete trajectory.
 */
bool ArmControlInterface::moveArmJoints(const RobotSide side, const std::vector<std::vector<double>> &arm_pose,
                                        const float time)
{
  ihmc_msgs::ArmTrajectoryRosMessage arm_traj;
  if (generateArmMessage(side, arm_pose, time, arm_traj))
  {
    armTrajectoryPublisher.publish(arm_traj);
    return true;
  }
  return false;
}

bool ArmControlInterface::setupArmMessage(const RobotSide side, ihmc_msgs::ArmTrajectoryRosMessage &msg)
{
  msg.joint_trajectory_messages.clear();
  msg.joint_trajectory_messages.resize(NUM_ARM_JOINTS);
  msg.robot_side = side;
  msg.unique_id = id_++;
}

bool ArmControlInterface::generateArmMessage(const RobotSide side, const std::vector<std::vector<double>> &arm_pose,
                                             const float time, ihmc_msgs::ArmTrajectoryRosMessage &msg)
{
  setupArmMessage(side, msg);
  int index_from_start;
  for (auto i = arm_pose.begin(); i != arm_pose.end(); i++)
  {
    if (i->size() != NUM_ARM_JOINTS)
    {
      ROS_WARN("Check number of trajectory points");
      return false;
    }
    index_from_start = (std::distance(arm_pose.begin(), i) + 1);
    appendTrajectoryPoint(msg, time / arm_pose.size() * index_from_start, *i);
  }
  return true;
}

void ArmControlInterface::generateArmMessage(
    const RobotSide side, const std::vector<ihmc_msgs::OneDoFJointTrajectoryRosMessage> &arm_trajectory,
    ihmc_msgs::ArmTrajectoryRosMessage &msg)
{
  msg.execution_mode = ihmc_msgs::ArmTrajectoryRosMessage::OVERRIDE;
  msg.joint_trajectory_messages = arm_trajectory;
  msg.robot_side = side;
  msg.unique_id = id_++;
}

/// TODO: It might be a good idea to shift this to whole body control message
/**
 * @brief ArmControlInterface::moveArmJoints moves both the arms together.
 * @param arm_data is the combined data of both arms
 */
bool ArmControlInterface::moveArmJoints(const std::vector<ArmJointData> &arm_data)
{
  ihmc_msgs::ArmTrajectoryRosMessage arm_traj_r;
  ihmc_msgs::ArmTrajectoryRosMessage arm_traj_l;
  bool right = false, left = false;

  arm_traj_r.joint_trajectory_messages.clear();
  arm_traj_r.joint_trajectory_messages.resize(NUM_ARM_JOINTS); /// check resize issue

  arm_traj_r.unique_id = id_++;

  arm_traj_l.joint_trajectory_messages.clear();
  arm_traj_l.joint_trajectory_messages.resize(NUM_ARM_JOINTS); /// check resize issue
  arm_traj_l.unique_id = id_++;

  for (auto i = arm_data.begin(); i != arm_data.end(); i++)
  {
    if (i->arm_pose.size() != NUM_ARM_JOINTS)
    {
      ROS_INFO("Check number of trajectory points");
      return false;
    }

    if (i->side == RIGHT)
    {
      right = true;
      arm_traj_r.robot_side = i->side;
      appendTrajectoryPoint(arm_traj_r, i->time, i->arm_pose);
    }

    else
    {
      left = true;
      arm_traj_l.robot_side = i->side;
      appendTrajectoryPoint(arm_traj_l, i->time, i->arm_pose);
    }
  }

  if (right)
    armTrajectoryPublisher.publish(arm_traj_r);
  ros::Duration(0.02).sleep(); /// TODO: Might have to increase time for safety when executing on hardware.
  if (left)
    armTrajectoryPublisher.publish(arm_traj_l);

  return true;
}

/**
 * @brief ArmControlInterface::moveArmMessage publishes already developed arm trajectory message
 * @param msg is the ArmTrajectoryRosMessage
 */
void ArmControlInterface::moveArmMessage(const ihmc_msgs::ArmTrajectoryRosMessage &msg)
{
  this->armTrajectoryPublisher.publish(msg);
}

/**
 * @brief ArmControlInterface::getnumArmJoints
 * @return the number of joint in each hand.
 */
int ArmControlInterface::getnumArmJoints() const
{
  return NUM_ARM_JOINTS;
}

/**
 * @brief ArmControlInterface::appendTrajectoryPoint will append a joint trajectory point
 * @param msg is the reference of the current msg where point is to be appended.
 * @param point is the joint trajectory point.
 */
void ArmControlInterface::appendTrajectoryPoint(ihmc_msgs::ArmTrajectoryRosMessage &msg,
                                                const trajectory_msgs::JointTrajectoryPoint &point)
{
  if (point.positions.size() != NUM_ARM_JOINTS)
  {
    ROS_WARN("Check number of trajectory points. Recieved %d expected %d", (int)point.positions.size(), NUM_ARM_JOINTS);
    return;
  }

  std::vector<std::pair<double, double>> joint_limits_;
  joint_limits_ = msg.robot_side == LEFT ? joint_limits_left_ : joint_limits_right_;
  std::vector<std::string> joint_names;
  rd_->getLeftArmJointNames(joint_names);
  for (int i = 0; i < NUM_ARM_JOINTS; i++)
  {
    ihmc_msgs::TrajectoryPoint1DRosMessage p;
    ROS_INFO("Joint : %s - theta : %0.8f limits - <%0.3f %0.3f>", joint_names[i].c_str(), point.positions[i],
             joint_limits_[i].first, joint_limits_[i].second);
    //        point.positions[i] = point.positions[i] <= joint_limits_[i].first  ? joint_limits_[i].first :
    //        point.positions[i];
    //        point.positions[i] = point.positions[i] >= joint_limits_[i].second ? joint_limits_[i].second :
    //        point.positions[i];
    float position = point.positions[i];
    if (point.positions[i] <= joint_limits_[i].first)
    {
      std::cout << "wrapped lower point " << point.positions[i] << "\n";
      position = joint_limits_[i].first;
    }
    else if (point.positions[i] >= joint_limits_[i].second)
    {
      std::cout << "wrapped upper point " << point.positions[i] << "\n";
      position = joint_limits_[i].second;
    }
    p.time = point.time_from_start.toSec();
    p.position = position;
    p.velocity = point.velocities[i];
    p.unique_id = id_++;
    msg.joint_trajectory_messages[i].trajectory_points.push_back(p);
  }

  return;
}

/**
 * @brief ArmControlInterface::moveArmTrajectory  moves the arm based on joint trajectory (mainly used with MOVEIT)
 * @param side is the side of the arm
 * @param traj is the trajectory message
 */
void ArmControlInterface::moveArmTrajectory(const RobotSide side, const trajectory_msgs::JointTrajectory &traj)
{
  ihmc_msgs::ArmTrajectoryRosMessage arm_traj;
  arm_traj.joint_trajectory_messages.clear();

  //    arm_traj.joint_trajectory_messages.resize(NUM_ARM_JOINTS); old /// resolve size issue
  arm_traj.joint_trajectory_messages.resize(traj.points.size()); /// resolve size issue
  arm_traj.robot_side = side;
  arm_traj.unique_id = ArmControlInterface::id_++;

  for (auto i = traj.points.begin(); i < traj.points.end(); i++)
  {
    appendTrajectoryPoint(arm_traj, *i);
  }
  ROS_INFO("Publishing Arm Trajectory");
  armTrajectoryPublisher.publish(arm_traj);
}

// *******
/// TODO: FIND USAGE OR DELETE

// armside is used to determine which side to use.
// transforms the poses to the worldframe regardless.
// POSES MUST BE IN WORLD FRAME;
// Add conversion of posestamped to world frame if it not already in world frame
bool ArmControlInterface::generate_task_space_data(const std::vector<geometry_msgs::PoseStamped> &input_poses,
                                                   const RobotSide input_side, const float desired_time,
                                                   std::vector<ArmControlInterface::ArmTaskSpaceData> &arm_data_vector)
{
  float time_delta = desired_time == 0 ? 0 : desired_time / input_poses.size();
  for (int i = 0; i < input_poses.size(); i++)
  {
    geometry_msgs::PoseStamped input_pose = input_poses.at(i);
    ArmControlInterface::ArmTaskSpaceData task_space_data;
    task_space_data.side = input_side;
    task_space_data.pose = input_pose.pose;
    task_space_data.time = time_delta;

    arm_data_vector.push_back(task_space_data);
  }
  return true;
}

//**********

/**
 * @brief ArmControlInterface::moveArmJoint moves single joint and sets to target value
 * @param side is the side of the arm
 * @param jointNumber is the number order of joint in the chain (starts from 0)
 * @param targetAngle is the target angle to set the joint.
 * @return false if state informer cannot find current state of the arm. true otherwise.
 */
bool ArmControlInterface::moveArmJoint(const RobotSide side, int jointNumber, const float targetAngle, float time)
{
  ros::spinOnce(); // ensure that the joints are updated
  std::string param = side == LEFT ? "left_arm" : "right_arm";

  std::vector<double> positions;
  if (state_informer_->getJointPositions(param, positions))
  {
    for (auto i : positions)
    {
      std::cout << i << " ";
    }
    std::cout << std::endl;

    positions[jointNumber] = targetAngle;

    for (auto i : positions)
    {
      std::cout << i << " ";
    }
    std::cout << std::endl;

    std::vector<std::vector<double>> trajectory;
    trajectory.push_back(positions);
    moveArmJoints(side, trajectory, time);
    return true;
  }
  return false;
}

// ************ Mesages using HandTrajectoryRosMessage  ******************** //

void ArmControlInterface::poseToSE3TrajectoryPoint(const geometry_msgs::Pose &pose,
                                                   ihmc_msgs::SE3TrajectoryPointRosMessage &point)
{
  point.position.x = pose.position.x;
  point.position.y = pose.position.y;
  point.position.z = pose.position.z;
  point.orientation.x = pose.orientation.x;
  point.orientation.y = pose.orientation.y;
  point.orientation.z = pose.orientation.z;
  point.orientation.w = pose.orientation.w;
  point.unique_id = ArmControlInterface::id_++;
  return;
}

bool ArmControlInterface::nudgeArm(const RobotSide side, const direction drct, float nudgeStep)
{
  geometry_msgs::Pose palm_pose;

  std::string target_frame = side == LEFT ? rd_->getLeftEEFrame() : rd_->getRightEEFrame();

  if (!state_informer_->getCurrentPose(target_frame, palm_pose, rd_->getPelvisFrame()))
  {
    return false;
  }
  ROS_INFO("Palm pose in pelvis frame: x=%.2f y=%.2f z=%.2f", palm_pose.position.x, palm_pose.position.y,
           palm_pose.position.z);

  if (drct == direction::LEFT)
    palm_pose.position.y += nudgeStep;
  else if (drct == direction::RIGHT)
    palm_pose.position.y -= nudgeStep;
  else if (drct == direction::UP)
    palm_pose.position.z += nudgeStep;
  else if (drct == direction::DOWN)
    palm_pose.position.z -= nudgeStep;
  else if (drct == direction::FRONT)
    palm_pose.position.x += nudgeStep;
  else if (drct == direction::BACK)
    palm_pose.position.x -= nudgeStep;

  //    //Translation
  //    palm_pose.position.x += 0.1;
  //    palm_pose.position.z -= 0.01;

  //    //Rotation
  //    tf::Quaternion q_orig, q_rot, q_new;
  //    double roll=0, pitch=0, yaw=M_PI_2;
  //    q_rot = tf::createQuaternionFromRPY(roll, pitch, yaw);

  //    tf::quaternionMsgToTF(palm_pose.orientation , q_orig);  // Get the original orientation of 'commanded_pose'

  //    q_new = q_rot*q_orig;  // Calculate the new orientation
  //    q_new.normalize();
  //    tf::quaternionTFToMsg(q_new, palm_pose.orientation);  // Stuff the new rotation back into the pose. This
  //    requires conversion into a msg type

  //    ROS_INFO("Palm pose in world frame: x=%.2f y=%.2f z=%.2f", palm_pose.position.x, palm_pose.position.y,
  //    palm_pose.position.z);

  moveArmInTaskSpace(side, palm_pose, 0.0f);

  return true;
}

bool ArmControlInterface::nudgeArmLocal(const RobotSide side, const direction drct, float nudgeStep)
{
  std::string target_frame = side == LEFT ? rd_->getLeftPalmFrame() : rd_->getRightPalmFrame();
  int signInverter = side == LEFT ? 1 : -1;

  geometry_msgs::Pose value;
  state_informer_->getCurrentPose(target_frame, value);
  std::cout << "Before-> World Frame x: " << value.position.x << " y: " << value.position.y
            << " z: " << value.position.z << "\n";
  state_informer_->transformPose(value, value, rd_->getWorldFrame(), target_frame);
  std::cout << "Before-> Local Frame x: " << value.position.x << " y: " << value.position.y
            << " z: " << value.position.z << "\n";

  if (drct == direction::FRONT)
    value.position.y += nudgeStep * signInverter;
  else if (drct == direction::BACK)
    value.position.y -= nudgeStep * signInverter;
  else if (drct == direction::UP)
    value.position.z += nudgeStep;
  else if (drct == direction::DOWN)
    value.position.z -= nudgeStep;
  else if (drct == direction::LEFT)
    value.position.x += nudgeStep * signInverter;
  else if (drct == direction::RIGHT)
    value.position.x -= nudgeStep * signInverter;
  std::cout << "After -> Local Frame x: " << value.position.x << " y: " << value.position.y
            << " z: " << value.position.z << "\n";
  state_informer_->transformPose(value, value, target_frame, rd_->getWorldFrame());
  std::cout << "After -> World Frame x: " << value.position.x << " y: " << value.position.y
            << " z: " << value.position.z << "\n";
  moveArmInTaskSpace(side, value, 0.0f);
  return true;
}

bool ArmControlInterface::getJointSpaceState(std::vector<double> &joints, RobotSide side)
{
  if (side == RobotSide::LEFT)
  {
    return state_informer_->getJointPositions("left_arm", joints);
  }
  else if (side == RobotSide::RIGHT)
  {
    return state_informer_->getJointPositions("right_arm", joints);
  }
  return false;
}

bool ArmControlInterface::getTaskSpaceState(geometry_msgs::Pose &pose, RobotSide side, std::string fixedFrame)
{
  if (side == RobotSide::LEFT)
  {
    return state_informer_->getCurrentPose(rd_->getLeftEEFrame(), pose);
  }
  else if (side == RobotSide::RIGHT)
  {
    return state_informer_->getCurrentPose(rd_->getRightEEFrame(), pose, fixedFrame);
  }
  return false;
}

void ArmControlInterface::moveArmInTaskSpace(const RobotSide side, const geometry_msgs::Pose &pose, const float time,
                                             int baseForControl)
{
  ihmc_msgs::SE3TrajectoryPointRosMessage point;
  poseToSE3TrajectoryPoint(pose, point);
  point.time = time;
  this->moveArmInTaskSpaceMessage(side, point, baseForControl);
}

void ArmControlInterface::moveArmInTaskSpaceMessage(const RobotSide side,
                                                    const ihmc_msgs::SE3TrajectoryPointRosMessage &point,
                                                    int baseForControl)
{
  ihmc_msgs::HandTrajectoryRosMessage msg;
  // ihmc_msgs::FrameInformationRosMessage reference_frame;

  // not supported in 0.8.2
  // reference_frame.data_reference_frame_id = baseForControl;
  // reference_frame.trajectory_reference_frame_id = baseForControl;

  msg.robot_side = side;
  // msg.frame_information = reference_frame;
  msg.base_for_control = ihmc_msgs::HandTrajectoryRosMessage::CHEST;
  msg.taskspace_trajectory_points.push_back(point);
  msg.execution_mode = msg.OVERRIDE;

  msg.unique_id = ArmControlInterface::id_++;
  taskSpaceTrajectoryPublisher.publish(msg);
}

void ArmControlInterface::moveArmInTaskSpace(const std::vector<ArmTaskSpaceData> &arm_data, const int baseForControl)
{
  ihmc_msgs::HandTrajectoryRosMessage msg_l;
  ihmc_msgs::HandTrajectoryRosMessage msg_r;

  // not supported in 0.8.2
  // ihmc_msgs::FrameInformationRosMessage reference_frame;
  // reference_frame.data_reference_frame_id = baseForControl;
  // reference_frame.trajectory_reference_frame_id = baseForControl;

  msg_l.taskspace_trajectory_points.clear();
  msg_r.taskspace_trajectory_points.clear();

  msg_l.unique_id = ArmControlInterface::id_++;
  // msg_l.frame_information = reference_frame;
  msg_l.execution_mode = msg_l.OVERRIDE;

  msg_r.unique_id = ArmControlInterface::id_++;
  // msg_r.frame_information = reference_frame;
  msg_r.execution_mode = msg_r.OVERRIDE;

  for (auto i = arm_data.begin(); i != arm_data.end(); i++)
  {
    if (i->side == RIGHT)
    {
      msg_r.robot_side = i->side;
      ihmc_msgs::SE3TrajectoryPointRosMessage point;
      poseToSE3TrajectoryPoint(i->pose, point);
      point.time = i->time;
      msg_r.taskspace_trajectory_points.push_back(point);
    }

    else
    {
      msg_l.robot_side = i->side;
      ihmc_msgs::SE3TrajectoryPointRosMessage point;
      poseToSE3TrajectoryPoint(i->pose, point);
      point.time = i->time;
      msg_l.taskspace_trajectory_points.push_back(point);
    }
  }

  taskSpaceTrajectoryPublisher.publish(msg_r);
  ros::Duration(0.02).sleep();
  taskSpaceTrajectoryPublisher.publish(msg_l);
}
