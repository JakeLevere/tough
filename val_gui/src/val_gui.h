#ifndef FALLRISK_GUI_H
#define FALLRISK_GUI_H

// QT
#include <QMainWindow>
#include <QWidget>
#include <QEvent>
#include <QKeyEvent>
#include <QStatusBar>
#include <QImage>
#include <QPainter>
#include <QLabel>
#include <QTabWidget>
#include <QCheckBox>
#include <QSlider>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QPushButton>

// rviz
#include "rviz/visualization_manager.h"
#include "rviz/render_panel.h"
#include "rviz/display.h"
#include "rviz/panel.h"
#include "rviz/default_plugin/tools/measure_tool.h"
#include "rviz/tool_manager.h"
#include "rviz/default_plugin/tools/point_tool.h"

// ros
#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <std_msgs/Float32.h>
#include <sensor_msgs/Image.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>

// opencv/PCL
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

// custom Valkyrie APIs
#include <val_control/val_arm_navigation.h>
#include <val_control/val_chest_navigation.h>
#include <val_control/val_pelvis_navigation.h>
#include <val_footstep/ValkyrieWalker.h>

// Constants
#define CHEST_ROLL_MAX 14.61    // this is in degrees. coz I dont talk radians :P
#define CHEST_ROLL_MIN -13
#define CHEST_PITCH_MAX 38
#define CHEST_PITCH_MIN -7
#define CHEST_YAW_MAX 67
#define CHEST_YAW_MIN -76

#define PELVIS_HEIGHT_MAX 1.16
#define PELVIS_HEIGHT_MIN 0.8

#define RIGHT_SHOULDER_ROLL_MAX 66.42
#define RIGHT_SHOULDER_ROLL_MIN -72.56
#define RIGHT_SHOULDER_PITCH_MAX 114.64
#define RIGHT_SHOULDER_PITCH_MIN -163.37
#define RIGHT_SHOULDER_YAW_MAX 124.96
#define RIGHT_SHOULDER_YAW_MIN -177.70

#define LEFT_SHOULDER_ROLL_MAX 72.56
#define LEFT_SHOULDER_ROLL_MIN -66.42
#define LEFT_SHOULDER_PITCH_MAX 114.64
#define LEFT_SHOULDER_PITCH_MIN -163.37
#define LEFT_SHOULDER_YAW_MAX 124.96
#define LEFT_SHOULDER_YAW_MIN -177.70

#define RIGHT_WRIST_ROLL_MAX 35.53
#define RIGHT_WRIST_ROLL_MIN -35.82
#define RIGHT_WRIST_PITCH_MAX 20.63
#define RIGHT_WRIST_PITCH_MIN -28.08
#define RIGHT_WRIST_YAW_MAX 180
#define RIGHT_WRIST_YAW_MIN -115.73

#define LEFT_WRIST_ROLL_MAX 35.82
#define LEFT_WRIST_ROLL_MIN -35.53
#define LEFT_WRIST_PITCH_MAX 28.08
#define LEFT_WRIST_PITCH_MIN -20.63
#define LEFT_WRIST_YAW_MAX 180
#define LEFT_WRIST_YAW_MIN -177.70


#define LOWER_NECK_PITCH_MAX 66.61
#define LOWER_NECK_PITCH_MIN 0
#define UPPER_NECK_PITCH_MAX 0
#define UPPER_NECK_PITCH_MIN -49.9
#define NECK_YAW_MAX 60
#define NECK_YAW_MIN -60

#define LEFT_ELBOW_MAX 6.87
#define LEFT_ELBOW_MIN -124.38

#define RIGHT_ELBOW_MAX 124.61
#define RIGHT_ELBOW_MIN -6.87

#define BASE_BATTERY_CAP 165
#define BASE_BATTERY_LOW 140
#define BASE_BATTERY_DANGER 132

namespace Ui {
class ValkyrieGUI;
}

namespace rviz
{
class Display;
class RenderPanel;
class VisualizationManager;
class ImageDisplay;
class Panel;
}

class ValkyrieGUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit ValkyrieGUI(QWidget *parent = 0);
    ~ValkyrieGUI();


private:
    Ui::ValkyrieGUI *ui;

private:
    void initActionsConnections();
    void initDisplayWidgets();
    void initVariables();
    void initTools();
    void initDefaultValues();
    void initValkyrieControllers();
    void sendMoveBaseCmd();

private Q_SLOTS:
    void closeGrippers();
    void openGrippers();
    void keyPressEvent(QKeyEvent *event);
    void setRobotVelocity();
    void setCurrentTool(int btnID);
    void setActiveRvizToolBtns(int tabID);
    void displayPointcloud(int btnID);
    void setStartFoot(int btnID);
    void moveChestJoints();
    void walkSteps();
    void changePelvisHeight();


private:
  rviz::VisualizationManager* manager_;
  rviz::VisualizationManager* mapManager_;

  rviz::RenderPanel* renderPanel_;
  rviz::RenderPanel* mapRenderPanel_ ;
  rviz::RenderPanel* imagePanel_;

  rviz::ViewManager* mapViewManager_;
  rviz::ViewController* mapViewController_ ;

  rviz::Display* cloudDisplay_;
//  rviz::Display* imageDisplay_;
  rviz::Display* octomapDisplay_;
  rviz::Display* mapDisplay_ ;

  rviz::ToolManager* toolManager_ ;
  rviz::ToolManager* mapToolManager_ ;

  rviz::Tool* measureTool_ ;
  rviz::Tool* pointTool_ ;
  rviz::Tool* interactTool_;
  rviz::Tool* mapInteractTool_;
  rviz::Tool* setGoalTool_;
  rviz::Tool* setMapGoalTool_;
  rviz::Tool* setInitialPoseTool_;
  rviz::Tool* setMapInitialPoseTool_;

private:
  ros::NodeHandle nh_;
  ros::Publisher moveBaseCmdPub;
  ros::Subscriber centerDistSub;
  ros::Subscriber baseSensorStatus;
  ros::Subscriber rviz2DNavGoalSub;

  image_transport::ImageTransport it_;
  image_transport::Subscriber liveVideoSub;

  chestTrajectory  *chestController_;
  pelvisTrajectory *pelvisHeightController_;
  armTrajectory    *armJointController_;
  ValkyrieWalker   *walkingController_;

  geometry_msgs::Twist moveBaseCmd;
  bool walkfoot_;
  float linearVelocity_;
  float angularVelocity_;
  float shoulderpitchslider_;
  float shoulderrollslider_;
  float shoulderyawslider_;
  float wristpitchslider_;
  float wristrollslider_;
  float wristyawslider_;
  float neckpitchslider_;
  float neckrollslider_;
  float neckyawslider_;
  float elbowslider_;

  void distanceSubCallback(const std_msgs::Float32::ConstPtr& msg);
//  void baseStatusCheck(const kobuki_msgs::SensorState::ConstPtr& msg);
  void liveVideoCallback(const sensor_msgs::ImageConstPtr &msg);
  void setVideo(QLabel* label, cv_bridge::CvImagePtr cv_ptr,bool is_RGB);

  void changeToolButtonStatus(int btnID);

  QString fixedFrame_;
  QString targetFrame_ ;
  QString mapTopic_;
  QString imageTopic_;
  QString pointCloudTopic_;
  QString octomapTopic_;
  QString baseSensorTopic_;
  QString velocityTopic_;
  QString pathTopic_;
  QString robotType_;
  QLabel* status_label_;

};


#endif // FALLRISK_GUI_H