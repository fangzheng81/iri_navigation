cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)

set(PROJECT_NAME ackermann_local_planner_alg_node)

# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
#set(ROS_BUILD_TYPE RelWithDebInfo)

rosbuild_init()

#set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#uncomment if you have defined messages
rosbuild_genmsg()
#uncomment if you have defined services
#rosbuild_gensrv()

# added to include support for dynamic reconfiguration
rosbuild_find_ros_package(dynamic_reconfigure)
include(${dynamic_reconfigure_PACKAGE_PATH}/cmake/cfgbuild.cmake)
gencfg()
# end dynamic reconfiguration

find_package(Eigen)
include_directories(${EIGEN_INCLUDE_DIRS})
add_definitions(${EIGEN_DEFINITIONS})

FIND_PACKAGE(iriutils REQUIRED)

INCLUDE_DIRECTORIES(${iriutils_INCLUDE_DIR} ./include)

#common commands for building c++ executables and libraries
rosbuild_add_library(${PROJECT_NAME} src/goal_functions.cpp src/map_cell.cpp src/map_grid.cpp src/point_grid.cpp src/costmap_model.cpp src/voxel_grid_model.cpp src/trajectory_planner.cpp src/trajectory.cpp src/trajectory_planner_ros.cpp src/map_grid_visualizer.cpp)
#target_link_libraries(${PROJECT_NAME} another_library)
rosbuild_add_boost_directories()
rosbuild_link_boost(${PROJECT_NAME} thread)
#rosbuild_add_executable(${PROJECT_NAME} src/ackermann_local_planner_alg.cpp src/ackermann_local_planner_alg_node.cpp)

target_link_libraries(${PROJECT_NAME} ${iriutils_LIBRARY})