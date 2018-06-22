// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Demonstration of a steering path-follower PID controller.
//
// The vehicle reference frame has Z up, X towards the front of the vehicle, and
// Y pointing to the left.
// TO DO:
// Find proper yaw rate calculation
// Subsscribe to msg from obstacle_avoidance_chrono, callback to that to calculate ChBezierCurve
// =============================================================================

#include <fstream>
#include<iostream>
#include "boost/bind.hpp"
#include "chrono/core/ChFileutils.h"
#include "chrono/core/ChRealtimeStep.h"
#include "chrono/utils/ChFilters.h"
#include <ros/console.h>
#include <ros/callback_queue.h>
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "ros_chrono_msgs/veh_status.h"
#include "nloptcontrol_planner/Control.h"
#include <unistd.h>
#include <interpolation.h>
#include <PID.h>
#include <vector>
#include <math.h>
#include <string>
//#include "tf/tf.h"
#include <sstream>
#include "chrono_vehicle/ChVehicleModelData.h"
#include "chrono_vehicle/terrain/RigidTerrain.h"
#include "chrono_vehicle/driver/ChIrrGuiDriver.h"
#include "chrono_vehicle/driver/ChPathFollowerDriver.h"
#include "chrono_vehicle/wheeled_vehicle/utils/ChWheeledVehicleIrrApp.h"
#include "chrono_models/vehicle/hmmwv/HMMWV.h"
#include "chrono/physics/ChSystem.h"
#include "chrono_vehicle/wheeled_vehicle/tire/ChPacejkaTire.h"
//#include "chrono_models/vehicle/hmmwv/HMMWV_Pac02Tire.h"
#include "chrono_vehicle/wheeled_vehicle/tire/RigidTire.h"
#include "chrono_vehicle/wheeled_vehicle/tire/LugreTire.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_ReissnerTire.h"
#include "chrono_vehicle/ChSubsysDefs.h"
#include "chrono_vehicle/wheeled_vehicle/vehicle/WheeledVehicle.h"
#include "chrono_vehicle/powertrain/ChShaftsPowertrain.h"
#include "chrono_vehicle/powertrain/SimplePowertrain.h"
#include "chrono_thirdparty/rapidjson/document.h"
#include "chrono_thirdparty/rapidjson/filereadstream.h"

#include "chrono/core/ChFileutils.h"
#include "chrono/core/ChStream.h"
#include "chrono/utils/ChFilters.h"
#include "chrono/utils/ChUtilsInputOutput.h"
#include "chrono/solver/ChSolverMINRES.h"
#include "chrono_vehicle/ChConfigVehicle.h"

#define PI 3.1415926535
//using veh_status.msg
using namespace chrono;
using namespace chrono::geometry;
using namespace chrono::vehicle;
using namespace chrono::vehicle::hmmwv;

using namespace alglib;

using namespace rapidjson;
// =============================================================================
// Problem parameters
// Main Data Path TODO fix this! should be a rosparam!
//std::string data_path("/home/shreyas/.julia/v0.6/MAVs/catkin_ws/data/vehicle/");
std::string data_path("../../../src/models/chrono/ros_chrono/src/data/vehicle/");

std::string chrono_data("../../../src/models/chrono/ros_chrono/src/data/");

//std::string data_path("src/system/chrono/ros_chrono/src/data/vehicle/");
// Contact method type
ChMaterialSurface::ContactMethod contact_method = ChMaterialSurface::NSC;

// Type of tire model (RIGID, LUGRE, FIALA, or PACEJKA)
// TireModelType tire_model = TireModelType::RIGID;

// Input file name for PACEJKA tires if they are selected
std::string pacejka_tire_file_left(data_path+"hmmwv/tire/HMMWV_pacejka_left.tir");
std::string pacejka_tire_file_right(data_path+"hmmwv/tire/HMMWV_pacejka_right.tir");
std::string lugre_tire_file(data_path+"hmmwv/tire/HMMWV_LugreTire.json");
std::string rigid_tire_file(data_path+"hmmwv/tire/HMMWV_RigidTire.json");
std::string reissner_tire_file(data_path+"hmmwv/tire/HMMWV_ReissnerTire.json");
//std::string pacejka_tire_file("hmmwv/tire/HMMWV_pacejka.tir");
// Type of powertrain model (SHAFTS or SIMPLE)
PowertrainModelType powertrain_model = PowertrainModelType::SHAFTS;

// Drive type (FWD, RWD, or AWD)
DrivelineType drive_type = DrivelineType::RWD;

// Visualization type for vehicle parts (PRIMITIVES, MESH, or NONE)
VisualizationType chassis_vis_type = VisualizationType::PRIMITIVES;
VisualizationType suspension_vis_type = VisualizationType::PRIMITIVES;
VisualizationType steering_vis_type = VisualizationType::PRIMITIVES;
VisualizationType wheel_vis_type = VisualizationType::NONE;
VisualizationType tire_vis_type = VisualizationType::PRIMITIVES;

std::string path_file(data_path+"paths/my_path.txt");
std::string simplepowertrain_file(data_path+"hmmwv/powertrain/HMMWV_ShaftsPowertrain.json");
WheelState wheel_states[4];


// Rigid terrain dimensions
double terrainHeight = 0;
double terrainLength = 500.0;  // size in X direction
double terrainWidth = 500.0;   // size in Y direction

// Point on chassis tracked by the chase camera
ChVector<> trackPoint(0.0, 0.0, 1.75);

// Simulation step size
double step_size = 1e-2;
double tire_step_size = step_size;

// Simulation end time
double t_end = 1000;

// Render FPS
double fps = 60;

// Debug logging
bool debug_output = false;
double debug_fps = 10;

// Output directories
const std::string out_dir = GetChronoOutputPath() + "STEERING_CONTROLLER";
const std::string pov_dir = out_dir + "/POVRAY";

// POV-Ray output
bool povray_output = false;

// Vehicle state output (forced to true if povray output enabled)
bool state_output = false;
int filter_window_size = 20;

// In src/data/vehicle/hmmwv/steering/HMMWV_PitmanArm.json revolution joint
double speed = 0.0;
double maximum_steering_angle;
// Controller output
double target_steering_interp = 0.0;
double target_speed_interp = 0.0;
double controller_output = 0.0;
double time_shift = 0.0;
PID controller;
std::vector<double> traj_t(2,0);
std::vector<double> traj_sa(2,0);
std::vector<double> traj_vx(2,0);

// =============================================================================

// Custom Irrlicht event receiver for selecting current driver model.
class ChDriverSelector : public irr::IEventReceiver {
  public:
    ChDriverSelector(const ChVehicle& vehicle, ChIrrGuiDriver* driver_gui)
        : m_vehicle(vehicle),
          m_driver_gui(driver_gui),
          m_driver(m_driver_gui),
          m_using_gui(true) {}

    ChDriver* GetDriver() { return m_driver; }
    bool UsingGUI() const { return m_using_gui; }

    virtual bool OnEvent(const irr::SEvent& event) {
        return false;
    }

  private:
    bool m_using_gui;
    const ChVehicle& m_vehicle;
    ChIrrGuiDriver* m_driver_gui;
    ChDriver* m_driver;
};

struct parameters
{
    RigidTerrain terrain;
    TireForces tire_forces;
    WheeledVehicle my_hmmwv;
    ChRealtimeStepTimer realtime_timer;
    int sim_frame;
    double steering_input;
    double throttle_input;
    double braking_input;
    std::vector<double> x_traj_curr;
    std::vector<double> y_traj_curr;
    std::vector<double> x_traj_prev;
    std::vector<double> y_traj_prev;
    double target_speed;
    int render_steps;
    int render_frame;
} ;

void waitForLoaded(ros::NodeHandle &n){
    bool planner_init = false;
    std::string planner_namespace;
    n.getParam("system/planner",planner_namespace);
    n.getParam("system/"+planner_namespace+"/flags/initialized",planner_init);
    while(!planner_init){
        usleep(500); // < my question
        n.getParam("system/"+planner_namespace+"/flags/initialized",planner_init);

    }
    usleep(500); //sleep another 500ms to ensure everything is loaded.
    //continue on here
}

void setChassisParams(ros::NodeHandle &n){
  std::ofstream myfile2;

  myfile2.open(data_path+"hmmwv/chassis/HMMWV_Chassis.json",std::ofstream::out | std::ofstream::trunc);
  std::string s1 = "{ ";
  std::string s2 = "  \"Name\":     \"HMMWV chassis\",";
  std::string s3 = "  \"Type\":     \"Chassis\",";
  std::string s4 = "  \"Template\": \"RigidChassis\",";
  std::string s5 = "  \"Components\":";
  std::string s6 = "  [";
  std::string s7 = "   {";
  std::string s8 = "     \"Centroidal Frame\":    {";
  std::string s9 = "                              \"Location\":    ";
  std::string s10;
  n.getParam("vehicle/chrono/vehicle_params/centroidLoc",s10);
  std::string s11 = "                              \"Orientation\": ";
  std::string s12;
  n.getParam("/vehicle/chrono/vehicle_params/centroidOrientation",s12);
  std::string s13 = "                            },";
  std::string s14 = "     \"Mass\":                ";
  std::string s15;
  n.getParam("/vehicle/chrono/vehicle_params/chassisMass",s15);
  std::string s16 = "     \"Moments of Inertia\":  ";
  std::string s17;
  n.getParam("/vehicle/chrono/vehicle_params/chassisInertia",s17);
  std::string s18 = "     \"Products of Inertia\": [0, 0, 0],";
  std::string s19 = "     \"Void\":                false";
  std::string s20 = "   }";
  std::string s21 = "  ],";
  std::string s22 = "  \"Driver Position\":";
  std::string s23 = "  {";
  std::string s24 = "    \"Location\":     ";
  std::string s25;
  n.getParam("/vehicle/chrono/vehicle_params/driverLoc",s25);
  std::string s26 = "    \"Orientation\":  ";
  std::string s27;
  n.getParam("/vehicle/chrono/vehicle_params/driverOrientation",s27);
  std::string s28 = "  },";
  std::string s29 = "  \"Visualization\":";
  std::string s30 = "  {";
  std::string s31 = "    \"Mesh\":";
  std::string s32 = "    {";
  std::string s33 = "       \"Filename\":  \"hmmwv/hmmwv_chassis.obj\",";
  std::string s34 = "       \"Name\":      \"hmmwv_chassis_POV_geom\"";
  std::string s35 = "    }";
  std::string s36 = "  }";
  std::string s37 = "}";
  myfile2 << s1 << '\n';
  myfile2 << s2 << '\n';
  myfile2 << s3 << '\n';
  myfile2 << s4 << '\n';
  myfile2 << '\n';
  myfile2 << s5  << '\n';
  myfile2 << s6  << '\n';
  myfile2 << s7  << '\n';
  myfile2 << s8  << '\n';
  myfile2 << s9 << s10  << '\n';
  myfile2 << s11 << s12  << '\n';
  myfile2 << s13  << '\n';
  myfile2 << s14 << s15  <<'\n';
  myfile2 << s16 << s17  << '\n';
  myfile2 << s18 << '\n';
  myfile2 << s19 << '\n';
  myfile2 << s20 << '\n';
  myfile2 << s21 << '\n';
  myfile2 << s22 << '\n';
  myfile2 << s23 << '\n';
  myfile2 << s24 << s25  << '\n';
  myfile2 << s26 << s27  << '\n';
  myfile2 << s28 << '\n';
  myfile2 << '\n';
  myfile2 << s29 << '\n';
  myfile2 << s30 << '\n';
  myfile2 << s31 << '\n';
  myfile2 << s32 << '\n';
  myfile2 << s33 << '\n';
  myfile2 << s34 << '\n';
  myfile2 << s35 << '\n';
  myfile2 << s36 << '\n';
  myfile2 << s37 << '\n';
  myfile2.close();
}

void setDrivelineParams(ros::NodeHandle &n){
  std::ofstream myfile2;

  myfile2.open(data_path+"hmmwv/driveline/HMMWV_Driveline2WD.json",std::ofstream::out | std::ofstream::trunc);
  std::string s1 = "{ ";
  std::string s2 = "  \"Name\":                       \"HMMWV RWD Driveline\",";
  std::string s3 = "  \"Type\":                       \"Driveline\",";
  std::string s4 = "  \"Template\":                   \"ShaftsDriveline2WD\",";
  std::string s5 = "  \"Shaft Direction\":";
  std::string s6 = "  {";
  std::string s7 = "    \"Motor Block\":              ";
  std::string s8;
  n.getParam("/vehicle/chrono/vehicle_params/motorBlockDirection",s8);
  std::string s9 = "    \"Axle\":                     ";
  std::string s10;
  n.getParam("/vehicle/chrono/vehicle_params/axleDirection",s10);
  std::string s11 = "  },";
  std::string s12 = "  \"Shaft Inertia\":";
  std::string s13 = "  {";
  std::string s14 = "    \"Driveshaft\":               ";
  std::string s15;
  n.getParam("/vehicle/chrono/vehicle_params/driveshaftInertia", s15);
  std::string s16 = "    \"Differential Box\":         ";
  std::string s17;
  n.getParam("/vehicle/chrono/vehicle_params/differentialBoxInertia",s17);
  std::string s18 = "  },";
  std::string s19 = "  \"Gear Ratio\":";
  std::string s20 = "  {";
  std::string s21 = "    \"Conical Gear\":             ";
  std::string s22;
  n.getParam("/vehicle/chrono/vehicle_params/conicalGearRatio",s22);
  std::string s23 = "    \"Differential\":             ";
  std::string s24;
  n.getParam("/vehicle/chrono/vehicle_params/differentialRatio",s24);
  std::string s25 = "  }";
  std::string s26 = "}";

  myfile2 << s1 << '\n';
  myfile2 << s2 << '\n';
  myfile2 << s3 << '\n';
  myfile2 << s4 << '\n';
  myfile2 << '\n';
  myfile2 << s5  << '\n';
  myfile2 << s6  << '\n';
  myfile2 << s7  << s8 << '\n';
  myfile2 << s9 << s10 << '\n';
  myfile2 << s11 << '\n';
  myfile2 << '\n';
  myfile2 << s12 << '\n';
  myfile2 << s13  << '\n';
  myfile2 << s14 << s15  << '\n';
  myfile2 << s16 << s17  << '\n';
  myfile2 << s18 << '\n';
  myfile2 << '\n';
  myfile2 << s19 << '\n';
  myfile2 << s20 << '\n';
  myfile2 << s21 << s22 << '\n';
  myfile2 << s23 << s24 << '\n';
  myfile2 << s25 << '\n';
  myfile2 << s26 ;
  myfile2.close();
}


void setSteeringParams(ros::NodeHandle &n){
  std::ofstream myfile2;

  myfile2.open(data_path+"hmmwv/steering/HMMWV_RackPinion.json",std::ofstream::out | std::ofstream::trunc);
  std::string s1 = "{";
  std::string s2 = "  \"Name\":                       \"HMMWV Rack-Pinion Steering\",";
  std::string s3 = "  \"Type\":                       \"Steering\",";
  std::string s4 = "  \"Template\":                   \"RackPinion\",";
  std::string s5 = "  \"Steering Link\":";
  std::string s6 = "  {";
  std::string s7 = "    \"Mass\":                     ";
  std::string s8 ;
  n.getParam("/vehicle/chrono/vehicle_params/steeringLinkMass", s8);
  std::string s9 = "    \"COM\":                      0,";
  std::string s10 = "    \"Inertia\":                  ";
  std::string s11;
  n.getParam("/vehicle/chrono/vehicle_params/steeringLinkInertia",s11);
  std::string s12 = "    \"Radius\":                   ";
  std::string s13;
  n.getParam("/vehicle/chrono/vehicle_params/steeringLinkRadius",s13);
  std::string s14 = "    \"Length\":                   ";
  std::string s15;
  n.getParam("/vehicle/chrono/vehicle_params/steeringLinkLength",s15);
  std::string s16 = "  },";
  std::string s17 = "  \"Pinion\":";
  std::string s18 = "    \"Radius\":                   ";
  std::string s19;
  n.getParam("/vehicle/chrono/vehicle_params/pinionRadius",s19);
  std::string s20 = "    \"Maximum Angle\":            ";
  std::string s21;
  n.getParam("/vehicle/chrono/vehicle_params/pinionMaxAngle",s21);
  std::string s22 = "  }";
  std::string s23 = "}";



  myfile2 << s1 << '\n';
  myfile2 << s2 << '\n';
  myfile2 << s3 << '\n';
  myfile2 << s4 << '\n';
  myfile2 << '\n';
  myfile2 << s5  << '\n';
  myfile2 << s6  << '\n';
  myfile2 << s7  << s8 << '\n';
  myfile2 << s9  << '\n';
  myfile2 << s10 << s11  << '\n';
  myfile2 << s12 << s13  << '\n';
  myfile2 << s14 << s15 << '\n';
  myfile2 << s16 << '\n';
  myfile2 << '\n';
  myfile2 << s17 << '\n';
  myfile2 << s18 << s19 << '\n';
  myfile2 << s20 << '\n';
  myfile2 << s21 << s21 << '\n';
  myfile2 << s22 << '\n';
  myfile2 << s23 << '\n';

  myfile2.close();
}

void setBrakingParams(ros::NodeHandle &n){
  std::ofstream myfile2;
  std::ofstream myfile3;

  std::string s1 = "{ ";
  std::string s2 = "  \"Name\":                       \"HMMWV Brake Front\",";
  std::string s3 = "  \"Type\":                       \"Brake\",";
  std::string s4 = "  \"Template\":                   \"BrakeSimple\",";
  std::string s5 = "  \"Maximum Torque\":             ";
  std::string s6;
  n.getParam("vehicle/chrono/vehicle_params/maxBrakeTorque",s6);
  std::string s7 = "}";
  std::string s8 = "  \"Name\":                       \"HMMWV Brake Rear\",";

  myfile2.open(data_path+"hmmwv/brake/HMMWV_BrakeSimple_Front.json",std::ofstream::out | std::ofstream::trunc);
  myfile2 << s1 << '\n';
  myfile2 << s2 << '\n';
  myfile2 << s3 << '\n';
  myfile2 << s4 << '\n';
  myfile2 << '\n';
  myfile2 << s5  << s6 << '\n';
  myfile2 << s7  << '\n';
  myfile2.close();

  myfile3.open(data_path+"hmmwv/brake/HMMWV_BrakeSimple_Rear.json",std::ofstream::out | std::ofstream::trunc);
  myfile3 << s1 << '\n';
  myfile3 << s8 << '\n';
  myfile3 << s2 << '\n';
  myfile3 << s3 << '\n';
  myfile3 << s4 << '\n';
  myfile3 << '\n';
  myfile3 << s5  << s6 << '\n';
  myfile3 << s7  << '\n';
  myfile3.close();
}

void controlCallback(const nloptcontrol_planner::Control::ConstPtr& control_msg){
  traj_t = control_msg->t;
  traj_sa = control_msg->sa;
  traj_vx = control_msg->vx;

  /*
  std::cout << "Current time: "<< time << std::endl;
  std::cout << "Time trajectory: " << " ";
  for(int i = 0; i < traj_t.size(); i++)
    std::cout << traj_t[i] << " ";
    std::cout << std::endl;

  std::cout << "Speed trajectory: " << " ";
  for(int i = 0; i < traj_vx.size(); i++)
      std::cout << traj_vx[i] << " ";
  std::cout << std::endl;

  std::cout << "Steering trajectory: " << " ";
  for(int i = 0; i < traj_sa.size(); i++)
      std::cout << traj_sa[i] << " ";
  std::cout << std::endl;
*/
  std::cout << "Target_speed: " << target_speed_interp << " Actual speed: " << speed << std::endl;
  // std::cout << "Target_steering: " << target_steering_interp << " Actual steering: " << target_steering_interp << std::endl;
  // std::cout << std::endl;
}

// =============================================================================
int main(int argc, char* argv[]) {

  //unsigned int microseconds = 10000000;
  //std::cout << "Sleeping for ten sec..." << std::endl;
  //usleep(microseconds);

  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
      fprintf(stdout, "Current working dir: %s\n", cwd);
  else
      perror("getcwd() error");
    GetLog() << "Copyright (c) 2017 projectchrono.org\nChrono version: " << CHRONO_VERSION << "\n\n";


    //fprintf(stdout, "CHRONO_DATA_DIR: %s\n", CHRONO_DATA_DIR);
    SetChronoDataPath(CHRONO_DATA_DIR);
    //SetChronoDataPath(chrono_data);
    vehicle::SetDataPath(data_path);

    // read maximum_steering_angle from json
    std::string filename(data_path + "hmmwv/steering/HMMWV_PitmanArm.json");
    FILE* fp = fopen(filename.c_str(), "r");
    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    fclose(fp);
    Document d;
    d.ParseStream<ParseFlag::kParseCommentsFlag>(is);
    maximum_steering_angle = d["Revolute Joint"]["Maximum Angle"].GetDouble();

    ros::init(argc, argv, "Traj_follower");
    ros::NodeHandle n;
    n.setParam("system/chrono/flags/initialized",true);
    n.getParam("system/params/step_size",step_size);

    bool planner_init;
  //  bool planner_init2;

    std::string planner_namespace;
    n.getParam("system/planner",planner_namespace);
    n.getParam("system/"+planner_namespace+"/flags/initialized",planner_init);
    std::cout << "The planner state:" << planner_init << std::endl;
    ros::Subscriber sub = n.subscribe(planner_namespace+"/control", 100, controlCallback);

    //planner_init2=planner_init1;
  //    n.getParam("system/"+planner_namespace+"/flags/initialized",planner_init2);

      if(!planner_init){
        waitForLoaded(n);
    }
  //std::string planner_init;

    n.getParam("system/"+planner_namespace+"/flags/initialized",planner_init);

    // Desired vehicle speed (m/s)
    double target_speed;
    n.getParam("state/chrono/X0/v_des",target_speed);

    //Initial Position
    double x0, y0, z0, yaw0, pitch0, roll0, goal_x, goal_y;;
    bool gui_switch;
    n.getParam("case/actual/X0/x",x0);
    n.getParam("case/actual/X0/yVal",y0);
    n.getParam("state/chrono/X0/z",z0);
    n.getParam("case/actual/X0/psi",yaw0);
    n.getParam("state/chrono/X0/theta",pitch0);
    n.getParam("state/chrono/X0/phi",roll0);
    n.getParam("case/goal/x",goal_x);
    n.getParam("case/goal/yVal",goal_y);

    n.getParam("system/chrono/flags/gui",gui_switch);

    bool goal_attained = false;
    bool running = true;

    setSteeringParams(n);
    setDrivelineParams(n);
    setBrakingParams(n);
    setChassisParams(n);
    ChVector<> initLoc(x0, y0, z0);
//    ChQuaternion<> initRot(q[0],q[1],q[2],q[3]);

    double t0 = std::cos(yaw0 * 0.5f);
    double t1 = std::sin(yaw0 * 0.5f);
    double t2 = std::cos(roll0 * 0.5f);
    double t3 = std::sin(roll0 * 0.5f);
    double t4 = std::cos(pitch0 * 0.5f);
    double t5 = std::sin(pitch0 * 0.5f);

    double q0_0 = t0 * t2 * t4 + t1 * t3 * t5;
    double q0_1 = t0 * t3 * t4 - t1 * t2 * t5;
    double q0_2 = t0 * t2 * t5 + t1 * t3 * t4;
    double q0_3 = t1 * t2 * t4 - t0 * t3 * t5;

    ChQuaternion<> initRot(q0_0,q0_1,q0_2,q0_3);
    //ChQuaternion<> initRot(cos(PI/4), 0, 0, sin(PI/4)); //initial yaw of pi/2

    ros::Publisher vehicleinfo_pub = n.advertise<ros_chrono_msgs::veh_status>("vehicleinfo", 1);

    //ros::Rate loop_rate(5);

    // ------------------------------
    // Create the vehicle and terrain
    // ------------------------------

    // Create the HMMWV vehicle, set parameters, and initialize
    std::cout << "Start reading the file" << std::endl;
    WheeledVehicle my_hmmwv(data_path + "hmmwv/vehicle/HMMWV_Vehicle.json",contact_method);
    my_hmmwv.Initialize(ChCoordsys<>(initLoc, initRot),0.0);

    std::cout << "Successfully read the file" << std::endl;
    my_hmmwv.SetChassisVehicleCollide(false);
    my_hmmwv.GetChassis()->SetVisualizationType(chassis_vis_type);
//    my_hmmwv.GetChassis()->AddVisualizationAssets(chassis_vis_type);

    my_hmmwv.SetSuspensionVisualizationType(suspension_vis_type);
    my_hmmwv.SetSteeringVisualizationType(steering_vis_type);
    my_hmmwv.SetWheelVisualizationType(wheel_vis_type);
    //my_hmmwv.SetTireVisualizationType(tire_vis_type);
    RigidTire tire_front_left(rigid_tire_file);
    RigidTire tire_front_right(rigid_tire_file);
    RigidTire tire_rear_left(rigid_tire_file);
    RigidTire tire_rear_right(rigid_tire_file);


    TireForces tire_forces(4);

    tire_front_left.Initialize(my_hmmwv.GetWheelBody(0), LEFT);
    tire_front_right.Initialize(my_hmmwv.GetWheelBody(1), LEFT);
    tire_rear_left.Initialize(my_hmmwv.GetWheelBody(2), RIGHT);
    tire_rear_right.Initialize(my_hmmwv.GetWheelBody(3), RIGHT);


    tire_front_left.SetVisualizationType(tire_vis_type);
    tire_front_right.SetVisualizationType(tire_vis_type);
    tire_rear_left.SetVisualizationType(tire_vis_type);
    tire_rear_right.SetVisualizationType(tire_vis_type);

    // Create the terrain
    float frict_coeff, rest_coeff;
    n.getParam("vehicle/common/frict_coeff",frict_coeff);
    n.getParam("vehicle/common/rest_coeff",rest_coeff);

    RigidTerrain terrain(my_hmmwv.GetSystem());
    my_hmmwv.GetWheel(0)->SetContactFrictionCoefficient(frict_coeff);
    my_hmmwv.GetWheel(0)->SetContactRestitutionCoefficient(rest_coeff);

    //terrain.SetContactFrictionCoefficient(0.9f);
    //terrain.SetContactRestitutionCoefficient(0.01f);
    terrain.SetContactMaterialProperties(2e7f, 0.3f);
    terrain.SetColor(ChColor(1, 1, 1));
    //terrain.SetTexture(chrono::vehicle::GetDataFile("terrain/textures/tile4.jpg"), 200, 200);
    terrain.SetTexture(data_path+"terrain/textures/tile4.jpg", 200, 200);
    terrain.Initialize(terrainHeight, terrainLength, terrainWidth);

    //SimplePowertrain powertrain(vehicle::GetDataFile("hmmwv/powertrain/HMMWV_SimplePowertrain.json"));
    HMMWV_Powertrain powertrain;
    powertrain.Initialize(my_hmmwv.GetChassisBody(),my_hmmwv.GetDriveshaft());
    std::vector<double> GearRatios;
    n.getParam("vehicle/chrono/vehicle_params/gearRatios",GearRatios);
    powertrain.SetGearRatios(GearRatios);
    // ---------------------------------------
    // Create the vehicle Irrlicht application
    // ---------------------------------------

    ChVehicleIrrApp app(&my_hmmwv, &powertrain, L"Steering Controller Demo",
                        irr::core::dimension2d<irr::u32>(800, 640));

    app.SetHUDLocation(500, 20);
    app.SetSkyBox();
    app.AddTypicalLogo();
    app.AddTypicalLights(irr::core::vector3df(-150.f, -150.f, 200.f), irr::core::vector3df(-150.f, 150.f, 200.f), 100,
                         100);
    app.AddTypicalLights(irr::core::vector3df(150.f, -150.f, 200.f), irr::core::vector3df(150.0f, 150.f, 200.f), 100,
                         100);
    app.EnableGrid(false);
    app.SetChaseCamera(trackPoint, 6.0, 0.5);

    app.SetTimestep(step_size);
    app.SetTryRealtime(true);

    // -------------------------
    // Create the driver systems
    // -------------------------

    // Create both a GUI driver and a path-follower and allow switching between them
    ChIrrGuiDriver driver_gui(app);
    driver_gui.Initialize();

    // ---------------
    // Simulation loop
    // ---------------

    // Driver location in vehicle local frame
    ChVector<> driver_pos = my_hmmwv.GetChassis()->GetLocalDriverCoordsys().pos;
    // Number of simulation steps between miscellaneous events
    double render_step_size = 1 / fps;
    int render_steps = (int)std::ceil(render_step_size / step_size);
    double debug_step_size = 1 / debug_fps;
    int debug_steps = (int)std::ceil(debug_step_size / step_size);

    // Initialize simulation frame counter and simulation time
    ChRealtimeStepTimer realtime_timer;
    int sim_frame = 0;
    int render_frame = 0;

    double throttle_input, steering_input, braking_input;
    std::vector<double> x_traj_curr, y_traj_curr,x_traj_prev,y_traj_prev; //Initialize xy trajectory vectors
    parameters hmmwv_params{terrain,tire_forces,my_hmmwv,realtime_timer,sim_frame,steering_input,throttle_input,braking_input,x_traj_curr,y_traj_curr,x_traj_prev,y_traj_prev,target_speed,render_steps,render_frame};
    //Load xy parameters for the first timestep
  //  std::string planner_namespace;
  //  n.getParam("system/planner",planner_namespace);

    // ----------------------
    // Create the Bezier path
    // ----------------------

    // Create and register a custom Irrlicht event receiver to allow selecting the
    // current driver model.
    ChDriverSelector selector(my_hmmwv, &driver_gui);
    app.SetUserEventReceiver(&selector);

    // Finalize construction of visualization assets
    app.AssetBindAll();
    app.AssetUpdateAll();

    // -----------------
    // Initialize output
    // -----------------

    state_output = state_output || povray_output;

    if (state_output) {
        if (ChFileutils::MakeDirectory(out_dir.c_str()) < 0) {
            std::cout << "Error creating directory " << out_dir << std::endl;
            return 1;
        }
    }

/*
    utils::CSV_writer csv("\t");
    csv.stream().setf(std::ios::scientific | std::ios::showpos);
    csv.stream().precision(6);
*/
    utils::ChRunningAverage fwd_acc_GC_filter(filter_window_size);
    utils::ChRunningAverage lat_acc_GC_filter(filter_window_size);

    utils::ChRunningAverage fwd_acc_driver_filter(filter_window_size);
    utils::ChRunningAverage lat_acc_driver_filter(filter_window_size);

    std::ofstream myfile1;
    myfile1.open(data_path+"paths/position.txt",std::ofstream::out | std::ofstream::trunc);
    //get mass and moment of inertia about z axis
    n.setParam("vehicle/common/m",my_hmmwv.GetVehicleMass());
    const ChMatrix33<> inertia_mtx= my_hmmwv.GetChassisBody()->GetInertia();
    double Izz=inertia_mtx.GetElement(2,2);
    n.setParam("vehicle/common/Izz",Izz);
    // get distance to front and rear axles
    // enum chrono::vehicle::VehicleSide LEFT;
    // enum chrono::vehicle::VehicleSide RIGHT;
    ChVector<> veh_com= my_hmmwv.GetVehicleCOMPos();
    ChVector<> la_pos=my_hmmwv.GetSuspension(0)->GetSpindlePos(chrono::vehicle::VehicleSide::RIGHT);
    ChVector<> lb_pos=my_hmmwv.GetSuspension(0)->GetSpindlePos(chrono::vehicle::VehicleSide::LEFT);
    double la_length, lb_length;
    ChVector<> la_diff;
    la_diff.Sub(veh_com,la_pos);
    ChVector<> lb_diff;
    lb_diff.Sub(veh_com,lb_pos);
    la_length=la_diff.Length();
    lb_length=lb_diff.Length();
    n.setParam("vehicle/common/la",la_length);
    n.setParam("vehicle/common/lb",lb_length);

    // get friction and restitution coefficients
  //  float frict_coeff, rest_coeff;
    frict_coeff = my_hmmwv.GetWheel(0)->GetCoefficientFriction();
    rest_coeff = my_hmmwv.GetWheel(0)->GetCoefficientRestitution();
    n.setParam("vehicle/common/frict_coeff",frict_coeff);
    n.setParam("vehicle/common/rest_coeff",rest_coeff);

    // set up the controller
    double Kp, Ki, Kd, Kw;
    std::string windup_method;
    n.getParam("controller/Kp",Kp);
    n.getParam("controller/Ki",Ki);
    n.getParam("controller/Kd",Kd);
    n.getParam("controller/Kw",Kw);
    n.getParam("controller/anti_windup",windup_method);
    n.getParam("controller/time_shift", time_shift);

    controller.set_PID(Kp, Ki, Kd, Kw);
    controller.set_step_size(step_size);
    controller.set_output_limit(-1.0, 1.0);
    controller.set_windup_metohd(windup_method);
    controller.initialize();
    ae_int_t natural_bound_type = 2;

    std::cout<< "Go into the loop..." << std::endl;

    while(n.ok()){
      while (running) {
        if(gui_switch)  app.GetDevice()->run();
        ros::spinOnce();
        n.setParam("system/chrono/flags/running",true);

        //     ros::Subscriber sub = n.subscribe<traj_gen_chrono::Control>("desired_ref", 1, &parameters::controlCallback, &hmmwv_params);

          // Render scene and output POV-Ray data
        if (hmmwv_params.sim_frame % hmmwv_params.render_steps == 0 && gui_switch) {

            app.BeginScene(true, true, irr::video::SColor(255, 140, 161, 192));
            app.DrawAll();
            app.EndScene();

            hmmwv_params.render_frame++;
          }

          ros_chrono_msgs::veh_status data_out;
  /*
          ChVector<> acc_CG = my_hmmwv.GetVehicle().GetChassisBody()->GetPos_dtdt();
          ChVector<> acc_driver = my_hmmwv.GetVehicle().GetVehicleAcceleration(driver_pos);
          double fwd_acc_CG = fwd_acc_GC_filter.Add(acc_CG.x());
          double lat_acc_CG = lat_acc_GC_filter.Add(acc_CG.y());
          double fwd_acc_driver = fwd_acc_driver_filter.Add(acc_driver.x());
          double lat_acc_driver = lat_acc_driver_filter.Add(acc_driver.y());*/
          double time = hmmwv_params.my_hmmwv.GetSystem()->GetChTime();

          // End simulation
          if (time >= t_end)
              break;

          real_1d_array t_array;
          real_1d_array steering_array;
          real_1d_array speed_array;

          t_array.setcontent(traj_t);
          steering_array.setcontent(traj_sa);
          speed_array.setcontent(traj_vx);
          spline1dinterpolant s_vx;
          spline1dinterpolant s_sa;
          spline1dbuildcubic(t_array, speed_array, s_vx);
          target_speed_interp = spline1dcalc(s_vx, time + time_shift);
          spline1dbuildcubic(t_array, steering_array, s_sa);
          target_steering_interp = spline1dcalc(s_sa, time);

          double speed_error = target_speed_interp - speed;
          controller_output = controller.control(speed_error);

          target_steering_interp = target_steering_interp / PI * 180.0 / maximum_steering_angle * 2;

          hmmwv_params.target_speed = target_speed_interp;

          // Hack for acceleration-braking maneuver
          /* // on-off controller
          bool braking = false;
          if (my_hmmwv.GetVehicle().GetVehicleSpeed() > hmmwv_params.target_speed)
              braking = true;
          if (braking) {
              hmmwv_params.throttle_input = 0;
              hmmwv_params.braking_input = 1;
          }
          else {
              hmmwv_params.throttle_input = 1;
              hmmwv_params.braking_input = 0;
          }
          */
          // pid controller
          if (controller_output > 0){
            hmmwv_params.throttle_input = controller_output;
            hmmwv_params.braking_input = 0;
          }
          else{
            hmmwv_params.throttle_input = 0;
            hmmwv_params.braking_input = -controller_output;
          }

          // Hack for steering maneuver
          hmmwv_params.steering_input = target_steering_interp;


          // std::cout << "Chrono steering input: " << hmmwv_params.steering_input <<std::endl;
          hmmwv_params.tire_forces[0] = tire_front_left.GetTireForce();
          hmmwv_params.tire_forces[1] = tire_front_right.GetTireForce();
          hmmwv_params.tire_forces[2] = tire_rear_left.GetTireForce();
          hmmwv_params.tire_forces[3] = tire_rear_right.GetTireForce();

          wheel_states[0] = hmmwv_params.my_hmmwv.GetWheelState(0);
          wheel_states[1] = hmmwv_params.my_hmmwv.GetWheelState(1);
          wheel_states[2] = hmmwv_params.my_hmmwv.GetWheelState(2);
          wheel_states[3] = hmmwv_params.my_hmmwv.GetWheelState(3);

          double x, y, goal_tol;
          n.getParam("state/chrono/x",x);
          n.getParam("state/chrono/yVal",y);
          n.getParam("system/params/goal_tol",goal_tol);
          double distance  = sqrt( (goal_x-x)*(goal_x-x) + (goal_y-y)*(goal_y-y) );
          if(distance < goal_tol){
            goal_attained = true;
            n.setParam("system/flags/goal_attained","true");
            // n.setParam("system/chrono/flags/running","false");
            running = false;
            hmmwv_params.braking_input = 1.0;
            hmmwv_params.throttle_input = 0.0;
            std::cout << "Goal attained!" << std::endl;
          }

          driver_gui.Synchronize(time);
          hmmwv_params.terrain.Synchronize(time);
          tire_front_left.Synchronize(time, wheel_states[0], hmmwv_params.terrain);
          tire_front_right.Synchronize(time, wheel_states[1], hmmwv_params.terrain);
          tire_rear_left.Synchronize(time, wheel_states[2], hmmwv_params.terrain);
          tire_rear_right.Synchronize(time, wheel_states[3], hmmwv_params.terrain);

          double powertrain_torque=powertrain.GetOutputTorque();
          double driveshaft_speed=my_hmmwv.GetDriveshaftSpeed();
          powertrain.Synchronize(time,hmmwv_params.throttle_input,driveshaft_speed);

          hmmwv_params.my_hmmwv.Synchronize(time, hmmwv_params.steering_input, hmmwv_params.braking_input, powertrain_torque, hmmwv_params.tire_forces);

          std::string msg = selector.UsingGUI() ? "GUI driver" : "Follower driver";
          if(gui_switch) app.Synchronize(msg, hmmwv_params.steering_input, hmmwv_params.throttle_input, hmmwv_params.braking_input);

          // Advance simulation for one timestep for all modules
          double step = hmmwv_params.realtime_timer.SuggestSimulationStep(step_size);
          driver_gui.Advance(step);
          powertrain.Advance(step);
          hmmwv_params.my_hmmwv.Advance(step);
          hmmwv_params.terrain.Advance(step);
          tire_front_right.Advance(tire_step_size);
          tire_front_left.Advance(tire_step_size);
          tire_rear_right.Advance(tire_step_size);
          tire_rear_left.Advance(tire_step_size);
          if(gui_switch) app.Advance(step);

          // Increment simulation frame number
          hmmwv_params.sim_frame++;

          ChVector<> global_pos = hmmwv_params.my_hmmwv.GetVehicleCOMPos();//global location of chassis reference frame origin
          ChQuaternion<> global_orntn = hmmwv_params.my_hmmwv.GetVehicleRot();//global orientation as quaternion
          ChVector<> rot_dt = hmmwv_params.my_hmmwv.GetChassisBody()->GetWvel_loc();//global orientation as quaternion
          ChVector<> global_velCOM = hmmwv_params.my_hmmwv.GetChassisBody()->GetPos_dt();
          ChVector<> global_accCOM = hmmwv_params.my_hmmwv.GetChassisBody()->GetPos_dtdt();
          //ChVector<> euler_ang = global_orntn.Q_to_Rotv(); //convert to euler angles
          //ChPacejkaTire<> slip_angle = GetSlipAngle()
          double slip_angle = tire_front_left.GetLongitudinalSlip();
          speed = hmmwv_params.my_hmmwv.GetVehicleSpeedCOM();

          double q0 = global_orntn[0];
          double q1 = global_orntn[1];
          double q2 = global_orntn[2];
          double q3 = global_orntn[3];

          double yaw_val=atan2(2*(q0*q3+q1*q2),1-2*(q2*q2+q3*q3));
          double theta_val=asin(2*(q0*q2-q3*q1));
          double phi_val= atan2(2*(q0*q1+q2*q3),1-2*(q1*q1+q2*q2));

          n.setParam("state/chrono/t",time); //time in chrono simulation
          n.setParam("state/chrono/x", global_pos[0]) ;
          n.setParam("state/chrono/yVal",global_pos[1]);
          n.setParam("state/chrono/ux",fabs(global_velCOM[0])); //speed measured at the origin of the chassis reference frame.
          n.setParam("state/chrono/v", global_velCOM[1]);
          n.setParam("state/chrono/ax", global_accCOM[0]);
          n.setParam("state/chrono/psi",yaw_val); //in radians
          n.setParam("state/chrono/theta",theta_val); //in radians
          n.setParam("state/chrono/phi",phi_val); //in radians
          n.setParam("state/chrono/r",-rot_dt[2]);//yaw rate
          n.setParam("state/chrono/sa",slip_angle); //slip angle
          n.setParam("state/chrono/control/thr",hmmwv_params.throttle_input); //throttle input in the range [0,+1]
          n.setParam("state/chrono/control/brk",hmmwv_params.braking_input); //braking input in the range [0,+1]
          n.setParam("state/chrono/control/str",hmmwv_params.steering_input); //steeering input in the range [-1,+1]

          n.setParam("system/simulation_time", time);

          data_out.t_chrono=time; //time in chrono simulation
          data_out.x_pos= global_pos[0];
          data_out.y_pos=global_pos[1];
          data_out.x_v= fabs(global_velCOM[0]); //speed measured at the origin of the chassis reference frame.
          data_out.y_v= global_velCOM[1];
          data_out.x_a= global_accCOM[0];
          data_out.yaw_curr=yaw_val; //in radians
          data_out.yaw_rate=-rot_dt[2];//yaw rate
          data_out.sa=slip_angle; //slip angle
          data_out.thrt_in=hmmwv_params.throttle_input; //throttle input in the range [0,+1]
          data_out.brk_in=hmmwv_params.braking_input; //braking input in the range [0,+1]
          data_out.str_in=hmmwv_params.steering_input; //steeering input in the range [-1,+1]

          vehicleinfo_pub.publish(data_out);
        //  loop_rate.sleep();

         myfile1 << ' ' << global_pos[0] << ' '<< global_pos[1]  <<' ' << global_pos[2]  << '\n';
         n.getParam("system/" + planner_namespace + "/flags/goal_attained",goal_attained);
         if(goal_attained){
           n.setParam("system/goal_attained","true");
           n.setParam("system/chrono/flags/running","false");
           running = false;
         }
      }
    }
/*
    if (state_output){
        csv.write_to_file(out_dir + "/state.out");
    }*/
myfile1.close();

    return 0;
}