cmake_minimum_required(VERSION 3.8)
project(rviz_2d_plot_plugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rcpputils REQUIRED)
find_package(rosidl_typesupport_cpp REQUIRED)
find_package(rosidl_typesupport_introspection_cpp REQUIRED)
find_package(rviz_common REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

add_library(${PROJECT_NAME} SHARED
  include/rviz_2d_plot_plugin/plot_2d_config.hpp
  include/rviz_2d_plot_plugin/plot_2d_controller.hpp
  include/rviz_2d_plot_plugin/plot_2d_display.hpp
  include/rviz_2d_plot_plugin/plot_path_resolver.hpp
  include/rviz_2d_plot_plugin/plot_range.hpp
  include/rviz_2d_plot_plugin/sample_buffer.hpp
  include/rviz_2d_plot_plugin/tick_generator.hpp
  include/rviz_2d_plot_plugin/topic_field_introspection.hpp
  src/plot_2d_config.cpp
  src/plot_2d_controller.cpp
  src/plot_2d_display.cpp
  src/plot_path_resolver.cpp
  src/plot_range.cpp
  src/sample_buffer.cpp
  src/tick_generator.cpp
  src/topic_field_introspection.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt5::Widgets
)

ament_target_dependencies(${PROJECT_NAME} PUBLIC
  pluginlib
  rclcpp
  rcpputils
  rosidl_typesupport_cpp
  rosidl_typesupport_introspection_cpp
  rviz_common
)

pluginlib_export_plugin_description_file(rviz_common plugin_description.xml)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  FILES plugin_description.xml
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY icons
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_lint_auto REQUIRED)
  find_package(geometry_msgs REQUIRED)
  find_package(std_msgs REQUIRED)

  ament_add_gtest(test_plugin_assets test/test_plugin_assets.cpp)
  target_compile_definitions(test_plugin_assets PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  )

  ament_add_gtest(test_plot_2d_config test/test_plot_2d_config.cpp)
  target_link_libraries(test_plot_2d_config ${PROJECT_NAME})

  ament_add_gtest(test_plot_path_resolver test/test_plot_path_resolver.cpp)
  target_link_libraries(test_plot_path_resolver ${PROJECT_NAME})

  ament_add_gtest(test_plot_2d_controller test/test_plot_2d_controller.cpp)
  target_link_libraries(test_plot_2d_controller ${PROJECT_NAME})
  ament_target_dependencies(test_plot_2d_controller rclcpp geometry_msgs)

  ament_add_gtest(test_plot_range test/test_plot_range.cpp)
  target_link_libraries(test_plot_range ${PROJECT_NAME})

  ament_add_gtest(test_sample_buffer test/test_sample_buffer.cpp)
  target_link_libraries(test_sample_buffer ${PROJECT_NAME})

  ament_add_gtest(test_topic_field_introspection test/test_topic_field_introspection.cpp)
  target_link_libraries(test_topic_field_introspection ${PROJECT_NAME})
  ament_target_dependencies(test_topic_field_introspection rclcpp geometry_msgs std_msgs)

  ament_add_gtest(test_tick_generator test/test_tick_generator.cpp)
  target_link_libraries(test_tick_generator ${PROJECT_NAME})

  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
