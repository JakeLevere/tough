#pragma once

#include <ros/ros.h>
#include <srcsim/Satellite.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Pose.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>
#include <val_common/val_common_names.h>
#include <chrono>
#include <ctime>
#include <std_msgs/Empty.h>
#include <srcsim/Task.h>
#include "ros/package.h"
#include <fstream>
#include "boost/date_time/posix_time/posix_time.hpp"



// minimum moment to determine the rotation direction
#define HANDLE_MINIMUM_MOVMENT_IN_RAD      0.0698132 //4 deg
#define HANDLE_CONSTANT_THRESHOLD_IN_RAD   0.0174533 //1 deg
#define HANDLE_CONSTANT_DEBOUNCE_TIME_SEC  10

// control time ouit (real time)
#define HANDLE_CONTROL_TIMEOUT_SEC 60 //sec

// handle positions
//!!! enum is not used as this will be used for vector acess
#define PITCH_KNOB_CENTER    0
#define PITCH_KNOB_HANDLE    1
#define YAW_KNOB_CENTER      2
#define YAW_KNOB_HANDLE      3

// previous grasp state
enum class prevGraspState {
    NOT_INITIALISED = 0,
    GRASP_PITCH_HANDLE,
    GRASP_YAW_HANDLE
};

// pitch yaw selection
enum class controlSelection {
    CONTROL_NOT_INITIALISED = 0,
    CONTROL_PITCH ,
    CONTROL_YAW
};

// Pitch/Yaw direction
enum class valueDirection {
    VALUE_NOT_INITIALISED = 0,
    VALUE_CONSTANT,
    VALUE_TOWARDS_TO_GOAL,
    VALUE_AWAY_TO_GOAL,
    VALUE_TOGGLING,
    VALUE_INCRSING,
    VALUE_DECRASING
};

enum class valueConstant {
    NOT_INITIALISED = 0,
    VALUE_NOT_CHANGING,
    VALUE_CHANGING
};

enum class handleDirection {
    ANTICLOCK_WISE = 0,
    CLOCK_WISE
};

class task1Utils {
private:
    ros::NodeHandle nh_;
    ros::Subscriber satellite_sub_;
    srcsim::Satellite msg_;
    ros::Subscriber task_status_sub_;

    ros::Publisher marker_pub_, reset_pointcloud_pub_;
    void satelliteMsgCB (const srcsim::Satellite &msg);
    int current_checkpoint_;
    srcsim::Task taskMsg;


public:
    task1Utils(ros::NodeHandle nh);
    ~task1Utils();

    bool isPitchCorrectNow(void);
    bool isYawCorrectNow(void);
    bool isPitchCompleted(void);
    bool isYawCompleted(void);
    double getPitchDiff (void);
    double getYawDiff (void);
    double getPitch (void);
    double getYaw (void);
    valueDirection getValueStatus(double current_value, controlSelection control);
    void getCircle3D (geometry_msgs::Point center, geometry_msgs::Point start,geometry_msgs::Quaternion orientation, const std::vector<float> planeCoeffs, std::vector<geometry_msgs::Pose> &points, handleDirection direction, float radius, int steps =10);
    void visulatise6DPoints(std::vector<geometry_msgs::Pose> &points);
    void clearPointCloud();
    void taskStatusSubCB(const srcsim::Task &msg);
    int getCurrentCheckpoint() const;
    boost::posix_time::ptime timeNow;
    std::string logFile;
};
