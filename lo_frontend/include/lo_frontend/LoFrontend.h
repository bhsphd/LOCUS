/*
Authors:
  - Matteo Palieri    (matteo.palieri@jpl.nasa.gov)
  - Benjamin Morrell  (benjamin.morrell@jpl.nasa.gov)
*/

#ifndef LO_FRONTEND_LO_FRONTEND_H
#define LO_FRONTEND_LO_FRONTEND_H

#include <chrono>
#include <core_msgs/PoseAndScan.h>
#include <diagnostic_msgs/DiagnosticArray.h>
#include <diagnostic_msgs/DiagnosticStatus.h>
#include <dynamic_reconfigure/Reconfigure.h>
#include <frontend_utils/CommonStructs.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_utils/GeometryUtilsROS.h>
#include <geometry_utils/Transform3.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/geometry/Rot3.h>
#include <math.h>
#include <message_filters/subscriber.h>
#include <nav_msgs/Odometry.h>
#include <parameter_utils/ParameterUtils.h>
#include <pcl/common/common.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>
#include <point_cloud_filter/PointCloudFilter.h>
#include <point_cloud_localization/PointCloudLocalization.h>
#include <point_cloud_mapper/IPointCloudMapper.h>
#include <point_cloud_mapper/PointCloudMapper.h>
#include <point_cloud_mapper/settings.h>
#include <point_cloud_odometry/PointCloudOdometry.h>
#include <ros/callback_queue.h>
#include <ros/ros.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/PointCloud2.h>
#include <std_msgs/Bool.h>
#include <std_msgs/Float64.h>
#include <std_msgs/String.h>
#include <std_msgs/Time.h>
#include <tf/message_filter.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_listener.h>
#include <tf2/transform_datatypes.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2_ros/message_filter.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <visualization_msgs/Marker.h>
class LoFrontend {
  friend class LoFrontendTest;

public:
  typedef sensor_msgs::Imu Imu;
  typedef nav_msgs::Odometry Odometry;
  typedef geometry_msgs::PoseStamped PoseStamped;
  typedef std::map<double, Imu> ImuBuffer;
  typedef std::map<double, Odometry> OdometryBuffer;
  typedef std::map<double, PoseStamped> PoseStampedBuffer;
  typedef Imu::ConstPtr ImuConstPtr;
  typedef Odometry::ConstPtr OdometryConstPtr;
  typedef PoseStamped::ConstPtr PoseStampedConstPtr;

  LoFrontend();
  ~LoFrontend();

  bool Initialize(const ros::NodeHandle& n, bool from_log);

  std::vector<ros::AsyncSpinner> setAsynchSpinners(ros::NodeHandle& _nh);

private:
  std::string robot_type_;

  const std::string tf_buffer_authority_;

  std::string name_;
  bool b_verbose_;

  bool LoadParameters(const ros::NodeHandle& n);
  bool RegisterCallbacks(const ros::NodeHandle& n, bool from_log);
  bool RegisterLogCallbacks(const ros::NodeHandle& n);
  bool RegisterOnlineCallbacks(const ros::NodeHandle& n);
  bool CreatePublishers(const ros::NodeHandle& n);

  message_filters::Subscriber<PointCloudF> lidar_sub_mf_;

  ros::Subscriber lidar_sub_;
  ros::Subscriber imu_sub_;
  ros::Subscriber odom_sub_;
  ros::Subscriber pose_sub_;

  void setImuSubscriber(ros::NodeHandle& _nh);
  void setOdomSubscriber(ros::NodeHandle& _nh);
  void setLidarSubscriber(ros::NodeHandle& _nh);

  // Timer for odometry callback
  double odom_pub_rate_;
  ros::Timer odom_pub_timer_;
  void PublishOdomOnTimer(const ros::TimerEvent& ev);
  void PublishOdometry(const geometry_utils::Transform3& odometry,
                       const Eigen::Matrix<double, 6, 6>& covariance,
                       const ros::Time stamp);
  ros::Publisher odometry_pub_;

  ros::Publisher diagnostics_pub_;

  tf2_ros::MessageFilter<PointCloudF>* lidar_odometry_filter_;

  void LidarCallback(const PointCloudF::ConstPtr& msg);
  void ImuCallback(const ImuConstPtr& imu_msg);
  void OdometryCallback(const OdometryConstPtr& odometry_msg);
  void PoseStampedCallback(const PoseStampedConstPtr& pose_stamped_msg);
  // Main msg callback queue
  ros::CallbackQueue imu_queue_;
  ros::CallbackQueue odom_queue_;
  ros::CallbackQueue lidar_queue_;

  int lidar_queue_size_;
  int imu_queue_size_;
  int odom_queue_size_;
  int pose_queue_size_;

  ImuBuffer imu_buffer_;
  OdometryBuffer odometry_buffer_;
  PoseStampedBuffer pose_stamped_buffer_;

  tf2_ros::Buffer tf2_ros_odometry_buffer_;

  geometry_utils::Transform3 latest_pose_;
  ros::Time latest_pose_stamp_;
  ros::Time latest_odom_stamp_;
  ros::Time stamp_transform_to_;
  bool b_first_odom_timer_ = true;
  double transform_wait_duration_;
  bool b_have_published_odom_ = false;

  int imu_buffer_size_limit_;
  int odometry_buffer_size_limit_;
  int pose_stamped_buffer_size_limit_;

  template <typename T1, typename T2>
  bool InsertMsgInBuffer(const T1& msg, T2& buffer);

  template <typename T>
  int CheckBufferSize(const T& buffer) const;

  template <typename T1, typename T2>
  bool GetMsgAtTime(const ros::Time& stamp, T1& msg, T2& buffer) const;

  double translation_threshold_kf_;
  double rotation_threshold_kf_;
  bool b_add_first_scan_to_key_;

  gtsam::Pose3 ToGtsam(const geometry_utils::Transform3& pose) const;
  geometry_utils::Transform3 last_keyframe_pose_;

  std::string fixed_frame_id_;
  std::string base_frame_id_;
  std::string imu_frame_id_;
  std::string bd_odom_frame_id_;

  bool LoadCalibrationFromTfTree();
  tf::TransformListener imu_T_base_listener_;
  Eigen::Affine3d I_T_B_;
  Eigen::Affine3d B_T_I_;
  Eigen::Quaterniond I_T_B_q_;

  PointCloudFilter filter_;
  PointCloudOdometry odometry_;
  PointCloudLocalization localization_;
  IPointCloudMapper::Ptr mapper_;
  std::string window_local_mapping_type_;

  bool b_publish_map_;
  int counter_;
  int map_publishment_meters_;

  bool b_pcld_received_;
  int pcld_seq_prev_;

  PointCloudF::Ptr msg_filtered_;
  PointCloudF::Ptr msg_transformed_;
  PointCloudF::Ptr msg_neighbors_;
  PointCloudF::Ptr msg_base_;
  PointCloudF::Ptr msg_fixed_;
  PointCloudF::Ptr mapper_unused_fixed_;
  PointCloudF::Ptr mapper_unused_out_;

  /*--------------
  Data integration
  --------------*/

  bool SetDataIntegrationMode();
  int data_integration_mode_;
  int max_number_of_calls_;

  // Imu
  void CheckImuFrame(const ImuConstPtr& imu_msg);
  bool CheckNans(const Imu& msg);
  Eigen::Quaterniond GetImuQuaternion(const Imu& imu_msg);
  bool b_convert_imu_to_base_link_frame_;
  bool b_imu_frame_is_correct_;
  bool b_use_imu_integration_;
  bool b_use_imu_yaw_integration_;
  bool b_imu_has_been_received_;
  int imu_number_of_calls_;
  Eigen::Quaterniond imu_quaternion_previous_;
  Eigen::Quaterniond imu_quaternion_change_;
  Eigen::Matrix3d GetImuDelta();
  Eigen::Matrix3d GetImuYawDelta();

  // Odometry
  bool b_use_odometry_integration_;
  bool b_odometry_has_been_received_;
  int odometry_number_of_calls_;
  tf::Transform odometry_pose_previous_;
  tf::Transform tf_transform_;
  tf::Vector3 tf_translation_;
  tf::Quaternion tf_quaternion_;
  tf::Transform GetOdometryDelta(const tf::Transform& odometry_pose) const;

  // PoseStamped
  bool b_use_pose_stamped_integration_;
  bool b_pose_stamped_has_been_received_;
  int pose_stamped_number_of_calls_;

  /*-----------------
  Open space detector
  ------------------*/

  int number_of_points_open_space_;
  bool b_adaptive_input_voxelization_{false};
  uint points_to_process_in_callback_{3001};

  /*-----------------
  BB based OSD
  ------------------*/
  void CalculateCrossSection(const PointCloudF::ConstPtr& msg);
  bool b_use_osd_;
  double osd_size_threshold_;
  PointF minPoint_;
  PointF maxPoint_;
  bool b_publish_xy_cross_section_;
  ros::Publisher xy_cross_section_pub_;
  ros::ServiceClient voxel_leaf_size_changer_srv_;
  int counter_voxel_{0};

  /* ----------------------------------
  Dynamic hierarchical data integration
  ---------------------------------- */

  ros::NodeHandle nl_;
  void SwitchToImuIntegration();

  /* -------------------------
  Flat Ground Assumption (FGA)
  ------------------------- */

  ros::Subscriber fga_sub_;
  void FlatGroundAssumptionCallback(const std_msgs::Bool& bool_msg);

  /* -------------------------
  Computation Time Profiling
  ------------------------- */

  bool b_enable_computation_time_profiling_;
  ros::Publisher lidar_callback_duration_pub_;

  ros::Time lidar_callback_start_;
  ros::Time scan_to_scan_start_;
  ros::Time scan_to_submap_start_;
  ros::Time approx_nearest_neighbors_start_;

  ros::Publisher scan_to_scan_duration_pub_;
  ros::Publisher scan_to_submap_duration_pub_;
  ros::Publisher approx_nearest_neighbors_duration_pub_;
  ros::Publisher dchange_voxel_pub_;

  /* -------------------------
  Ground Truth
  ------------------------- */

  std::string gt_point_cloud_filename_;
  bool b_run_with_gt_point_cloud_;
  void InitWithGTPointCloud(const std::string filename);

  /* -------------------------
  Diagnostics
  ------------------------- */
  bool publish_diagnostics_;

  ros::Publisher base_frame_pcld_pub_;

  /*--------------------------
  Map Sliding Window 2
  --------------------------*/
  bool b_enable_msw_;
  int box_filter_size_;
  int velocity_buffer_size_;
  int translation_threshold_msw_;
  double rotational_velocity_threshold_;
  double translational_velocity_threshold_;
  geometry_utils::Transform3 previous_pose_;
  geometry_utils::Transform3 last_refresh_pose_;
  ros::Time previous_stamp_;
  std::vector<double> translational_velocity_buffer_;
  std::vector<double> rotational_velocity_buffer_;

  /*------------------------------
  Low-rate odom interpolation flag
  -------------------------------*/
  bool b_interpolate_;

  /*------------------------------
  Lidar Scan Dropped Statistics
  -------------------------------*/
  void CheckMsgDropRate(const PointCloudF::ConstPtr& msg);
  int scans_dropped_;
  int statistics_time_window_;

  ros::Time statistics_start_time_;
  std::string statistics_verbosity_level_;

  // Debug utilities
  bool b_debug_transforms_;
  ros::Publisher time_difference_pub_;

  double wait_for_odom_transform_timeout_;

  /*----------------------------------
  Subscribe to localizer space monitor
  ----------------------------------*/

  bool b_is_open_space_;
  ros::Subscriber space_monitor_sub_;
  void SpaceMonitorCallback(const std_msgs::String& msg);
  double translation_threshold_closed_space_kf_;
  double rotation_threshold_closed_space_kf_;
  double translation_threshold_open_space_kf_;
  double rotation_threshold_open_space_kf_;

  dynamic_reconfigure::Reconfigure voxel_param;
  dynamic_reconfigure::DoubleParameter double_param;

  /*--------------- 
  Dynamic Switching
  ---------------*/
  
  double sensor_health_timeout_;
  ros::Time last_reception_time_odom_; 
  ros::Time last_reception_time_imu_; 
  bool IsOdomHealthy();
  bool IsImuHealthy();
  bool IntegrateSensors(const ros::Time& stamp);
  bool IntegrateOdom(const ros::Time& stamp);
  bool IntegrateImu(const ros::Time& stamp); 

};

#endif
