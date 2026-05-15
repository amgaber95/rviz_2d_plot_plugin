cmake_minimum_required(VERSION 3.20)
project(rviz_2d_plot_plugin)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
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
find_package(rviz_ogre_vendor REQUIRED)
find_package(rviz_rendering REQUIRED)

if(rviz_common_VERSION VERSION_GREATER_EQUAL 15.1)
  find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
  set(QT_VERSION_MAJOR 6)
else()
  find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
  set(QT_VERSION_MAJOR 5)
endif()

add_library(${PROJECT_NAME} SHARED
  include/rviz_2d_plot_plugin/plot_2d_config.hpp
  include/rviz_2d_plot_plugin/plot_2d_controller.hpp
  include/rviz_2d_plot_plugin/plot_2d_display.hpp
  include/rviz_2d_plot_plugin/plot_2d_renderer.hpp
  include/rviz_2d_plot_plugin/plot_path_resolver.hpp
  include/rviz_2d_plot_plugin/plot_range.hpp
  include/rviz_2d_plot_plugin/plot_value_formatter.hpp
  include/rviz_2d_plot_plugin/sample_buffer.hpp
  include/rviz_2d_plot_plugin/tick_generator.hpp
  include/rviz_2d_plot_plugin/topic_field_introspection.hpp
  src/overlay_backend.cpp
  src/plot_2d_config.cpp
  src/plot_2d_controller.cpp
  src/plot_2d_display.cpp
  src/plot_2d_display_options.cpp
  src/plot_2d_renderer.cpp
  src/plot_path_resolver.cpp
  src/plot_range.cpp
  src/plot_2d_subscription_manager.cpp
  src/plot_value_formatter.cpp
  src/sample_buffer.cpp
  src/tick_generator.cpp
  src/topic_field_introspection.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets
  rviz_ogre_vendor::OgreMain
  rviz_ogre_vendor::OgreOverlay
)

if(rclcpp_VERSION VERSION_LESS 25.0.0)
  target_compile_definitions(${PROJECT_NAME} PRIVATE SERVICE_TYPESUPPORT_UNAVAILABLE)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
  pluginlib::pluginlib
  rclcpp::rclcpp
  rcpputils::rcpputils
  rosidl_typesupport_cpp::rosidl_typesupport_cpp
  rosidl_typesupport_introspection_cpp::rosidl_typesupport_introspection_cpp
  rviz_common::rviz_common
  rviz_rendering::rviz_rendering
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
  target_link_libraries(test_plot_2d_controller ${geometry_msgs_TARGETS} rclcpp::rclcpp)

  foreach(display_test
      test_plot_2d_display_properties
      test_plot_2d_display_interactions
      test_plot_2d_display_rendering
      test_plot_2d_display_subscriptions
      test_plot_2d_display_topics)
    ament_add_gtest(${display_test} test/${display_test}.cpp)
    target_include_directories(${display_test} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
      ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
    target_link_libraries(${display_test} ${PROJECT_NAME})
    target_link_libraries(${display_test} ${geometry_msgs_TARGETS} ${std_msgs_TARGETS} rclcpp::rclcpp)

  endforeach()

  ament_add_gtest(test_plot_range test/test_plot_range.cpp)
  target_link_libraries(test_plot_range ${PROJECT_NAME})

  ament_add_gtest(test_plot_value_formatter test/test_plot_value_formatter.cpp)
  target_link_libraries(test_plot_value_formatter ${PROJECT_NAME})

  ament_add_gtest(test_plot_2d_renderer test/test_plot_2d_renderer.cpp)
  target_link_libraries(test_plot_2d_renderer ${PROJECT_NAME})

  ament_add_gtest(test_overlay_backend test/test_overlay_backend.cpp)
  target_include_directories(test_overlay_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  target_link_libraries(test_overlay_backend ${PROJECT_NAME})

  ament_add_gtest(test_sample_buffer test/test_sample_buffer.cpp)
  target_link_libraries(test_sample_buffer ${PROJECT_NAME})

  ament_add_gtest(test_topic_field_introspection test/test_topic_field_introspection.cpp)
  target_link_libraries(test_topic_field_introspection ${PROJECT_NAME})
  target_link_libraries(test_topic_field_introspection ${geometry_msgs_TARGETS} ${std_msgs_TARGETS} rclcpp::rclcpp)

  ament_add_gtest(test_tick_generator test/test_tick_generator.cpp)
  target_link_libraries(test_tick_generator ${PROJECT_NAME})

  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
