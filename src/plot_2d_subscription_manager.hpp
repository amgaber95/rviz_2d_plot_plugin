// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef PLOT_2D_SUBSCRIPTION_MANAGER_HPP_
#define PLOT_2D_SUBSCRIPTION_MANAGER_HPP_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <rclcpp/generic_subscription.hpp>
#include <rclcpp/qos.hpp>
#include <rclcpp/serialized_message.hpp>

#include "rviz_2d_plot_plugin/plot_2d_config.hpp"
#include "rviz_2d_plot_plugin/plot_2d_controller.hpp"

namespace rviz_2d_plot_plugin
{

using PlotSerializedMessageCallback =
  std::function<void (std::shared_ptr<rclcpp::SerializedMessage>)>;

using PlotGenericSubscriptionFactory =
  std::function<rclcpp::GenericSubscription::SharedPtr(
      const std::string &,
      const std::string &,
      rclcpp::QoS,
      PlotSerializedMessageCallback)>;

using PlotSubscriptionMessageHandler =
  std::function<void (const std::string &, std::shared_ptr<rclcpp::SerializedMessage>)>;

struct PlotSubscriptionTarget
{
  std::string topic;
  std::string type;
};

rclcpp::QoS qosProfileFromConfig(QoSConfig config);
std::vector<PlotSubscriptionTarget> subscriptionTargetsFromControllerState(
  const Plot2DControllerState & state);

class Plot2DSubscriptionManager
{
public:
  void setFactory(PlotGenericSubscriptionFactory factory);
  bool hasFactory() const;
  void clear();
  void subscribe(
    const std::vector<PlotSubscriptionTarget> & targets,
    QoSConfig qos,
    PlotSubscriptionMessageHandler on_message);

private:
  PlotGenericSubscriptionFactory factory_;
  std::vector<rclcpp::GenericSubscription::SharedPtr> subscriptions_;
};

}  // namespace rviz_2d_plot_plugin

#endif  // PLOT_2D_SUBSCRIPTION_MANAGER_HPP_
