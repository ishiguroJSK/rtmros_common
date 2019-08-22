// -*- C++ -*-
/*!
 * @file  HrpsysSeqStateROSBridge.h * @brief hrpsys seq state - ros bridge * @date  $Date$ 
 *
 * $Id$ 
 */
#ifndef HRPSYSSEQSTATEROSBRIDGE_H
#define HRPSYSSEQSTATEROSBRIDGE_H

#include "HrpsysSeqStateROSBridgeImpl.h"

// rtm
#include <rtm/CorbaNaming.h>

// ros
#include "ros/ros.h"
#include "rosgraph_msgs/Clock.h"
#include "std_msgs/Int32.h"
#include "sensor_msgs/JointState.h"
#include "nav_msgs/Odometry.h"
#include "geometry_msgs/WrenchStamped.h"
#include "actionlib/server/simple_action_server.h"
#include "control_msgs/FollowJointTrajectoryAction.h"
#ifdef USE_PR2_CONTROLLERS_MSGS
#include "pr2_controllers_msgs/JointTrajectoryAction.h"
#include "pr2_controllers_msgs/JointTrajectoryControllerState.h"
#else
#include "control_msgs/JointTrajectoryControllerState.h"
#endif
#include "dynamic_reconfigure/Reconfigure.h"
#include "hrpsys_ros_bridge/MotorStates.h"
#include "hrpsys_ros_bridge/ContactStatesStamped.h"
#include "diagnostic_msgs/DiagnosticArray.h"
#include "sensor_msgs/Imu.h"
#include "hrpsys_ros_bridge/SetSensorTransformation.h"
//ishiguro
#include <tf/transform_listener.h>

extern const char* hrpsysseqstaterosbridgeimpl_spec[];

class HrpsysSeqStateROSBridge  : public HrpsysSeqStateROSBridgeImpl
{
 public:
  HrpsysSeqStateROSBridge(RTC::Manager* manager) ;
  ~HrpsysSeqStateROSBridge();

  RTC::ReturnCode_t onInitialize();
  RTC::ReturnCode_t onFinalize();
  RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

  void onJointTrajectory(trajectory_msgs::JointTrajectory trajectory);
#ifdef USE_PR2_CONTROLLERS_MSGS
  void onJointTrajectoryActionGoal();
  void onJointTrajectoryActionPreempt();
#endif
  void onFollowJointTrajectoryActionGoal();
  void onFollowJointTrajectoryActionPreempt();
  void onTrajectoryCommandCB(const trajectory_msgs::JointTrajectoryConstPtr& msg);
  //for human tracker
  void onHumanTrackerCOMCommandCB(const geometry_msgs::PoseStampedConstPtr& msg);
  void onHumanTrackerRFCommandCB(const geometry_msgs::PoseStampedConstPtr& msg);
  void onHumanTrackerLFCommandCB(const geometry_msgs::PoseStampedConstPtr& msg);
  void onHumanTrackerRHCommandCB(const geometry_msgs::PoseStampedConstPtr& msg);
  void onHumanTrackerLHCommandCB(const geometry_msgs::PoseStampedConstPtr& msg);
  void onHumanTrackerHEADCommandCB(const geometry_msgs::PoseStampedConstPtr& msg);
  void onHumanTrackerZMPCommandCB(const geometry_msgs::PointStampedConstPtr& msg);
  void onHumanTrackerRFWCommandCB(const geometry_msgs::WrenchStampedConstPtr& msg);
  void onHumanTrackerLFWCommandCB(const geometry_msgs::WrenchStampedConstPtr& msg);
  bool sendMsg (dynamic_reconfigure::Reconfigure::Request &req,
                dynamic_reconfigure::Reconfigure::Response &res);
  bool setSensorTransformation(hrpsys_ros_bridge::SetSensorTransformation::Request& req,
                               hrpsys_ros_bridge::SetSensorTransformation::Response& res);
  void onFeedbackWrench(const ros::MessageEvent<geometry_msgs::WrenchStamped const>& event);

      private:
  ros::NodeHandle nh;
  ros::Publisher joint_state_pub, joint_controller_state_pub, mot_states_pub, diagnostics_pub, clock_pub, zmp_pub, ref_cp_pub, act_cp_pub, odom_pub, imu_pub, em_mode_pub, ref_contact_states_pub, act_contact_states_pub;
  ros::Subscriber trajectory_command_sub;
  std::vector<ros::Subscriber> fbwrench_subs;
  std::vector<ros::Publisher> fsensor_pub, cop_pub;
#ifdef USE_PR2_CONTROLLERS_MSGS
  actionlib::SimpleActionServer<pr2_controllers_msgs::JointTrajectoryAction> joint_trajectory_server;
#endif
  actionlib::SimpleActionServer<control_msgs::FollowJointTrajectoryAction> follow_joint_trajectory_server;
  ros::ServiceServer sendmsg_srv;
  ros::ServiceServer set_sensor_transformation_srv;
  bool interpolationp, use_sim_time, use_hrpsys_time;
  bool publish_sensor_transforms;
  tf::TransformBroadcaster br;

  coil::Mutex m_mutex;
  coil::TimeMeasure tm;

  std::string nameserver;
  std::string rootlink_name;

  ros::Subscriber clock_sub;
  //ishiguro
  ros::Subscriber htzmp_sub,htcom_sub,htrfw_sub,htlfw_sub,htrf_sub,htlf_sub,htrh_sub,htlh_sub,hthead_sub;
  ros::Publisher htcom_dbg_pub,htrf_dbg_pub,htlf_dbg_pub,htrh_dbg_pub,htlh_dbg_pub,hthead_dbg_pub;
  ros::Publisher rpcom_dbg_pub,rprf_dbg_pub,rplf_dbg_pub,rprh_dbg_pub,rplh_dbg_pub,rphead_dbg_pub,rpzmp_dbg_pub,rpdcp_dbg_pub,rpacp_dbg_pub;
  ros::Publisher invdyn_dbg_pub;
  tf::TransformListener ht_tf_listener;

  nav_msgs::Odometry prev_odom;
  bool prev_odom_acquired;
  hrp::Vector3 prev_rpy;
  void clock_cb(const rosgraph_msgs::ClockPtr& str) {};

  bool follow_action_initialized;

  boost::mutex tf_mutex;
  double tf_rate;
  ros::Timer periodic_update_timer;
  std::vector<geometry_msgs::TransformStamped> tf_transforms;
  void periodicTimerCallback(const ros::TimerEvent& event);

  // odometry relatives
  void updateOdometry(const hrp::Vector3 &trans, const hrp::Matrix33 &R, const ros::Time &stamp);

  // teleop relatives
  void updateOdometryTF(const ros::Time &stamp);

  // imu relatives
  void updateImu(tf::Transform &base, bool is_base_valid, const ros::Time &stamp);
  
  // sensor relatives
  void updateSensorTransform(const ros::Time &stamp);
  std::map<std::string, geometry_msgs::Transform> sensor_transformations;
  boost::mutex sensor_transformation_mutex;

  // whole body master slave relatives
  void updateWBMS(const ros::Time &stamp);
};


extern "C"
{
  DLL_EXPORT void HrpsysSeqStateROSBridgeInit(RTC::Manager* manager);
};

#endif // HRPSYSSEQSTATEROSBRIDGE_H

