// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "plot_2d_subscription_manager.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace rviz_2d_plot_plugin
{

rclcpp::QoS qosProfileFromConfig(QoSConfig config)
{
  config.repair();
  rclcpp::QoS qos(static_cast<std::size_t>(config.depth));

  switch (config.reliability) {
    case QoSReliability::SystemDefault:
      qos.reliability(rclcpp::ReliabilityPolicy::SystemDefault);
      break;
    case QoSReliability::Reliable:
      qos.reliable();
      break;
    case QoSReliability::BestEffort:
      qos.best_effort();
      break;
  }

  switch (config.durability) {
    case QoSDurability::SystemDefault:
      qos.durability(rclcpp::DurabilityPolicy::SystemDefault);
      break;
    case QoSDurability::Volatile:
      qos.durability_volatile();
      break;
    case QoSDurability::TransientLocal:
      qos.transient_local();
      break;
  }

  return qos;
}

std::vector<PlotSubscriptionTarget> subscriptionTargetsFromControllerState(
  const Plot2DControllerState & state)
{
  std::vector<PlotSubscriptionTarget> targets;
  for (const PlotSeriesControllerState & series : state.series) {
    if (series.status != PlotControllerStatus::Ok) {
      continue;
    }

    const PlotSubscriptionTarget target{series.topic, series.type};
    const auto duplicate = std::find_if(
      targets.begin(), targets.end(),
      [&target](const PlotSubscriptionTarget & existing) {
        return existing.topic == target.topic && existing.type == target.type;
      });
    if (duplicate == targets.end()) {
      targets.push_back(target);
    }
  }
  return targets;
}

void Plot2DSubscriptionManager::setFactory(PlotGenericSubscriptionFactory factory)
{
  factory_ = std::move(factory);
}

bool Plot2DSubscriptionManager::hasFactory() const
{
  return static_cast<bool>(factory_);
}

void Plot2DSubscriptionManager::clear()
{
  subscriptions_.clear();
}

void Plot2DSubscriptionManager::subscribe(
  const std::vector<PlotSubscriptionTarget> & targets,
  const QoSConfig qos,
  PlotSubscriptionMessageHandler on_message)
{
  subscriptions_.clear();
  if (!factory_) {
    return;
  }

  const rclcpp::QoS qos_profile = qosProfileFromConfig(qos);
  subscriptions_.reserve(targets.size());
  for (const PlotSubscriptionTarget & target : targets) {
    subscriptions_.push_back(
      factory_(
        target.topic,
        target.type,
        qos_profile,
        [on_message, topic = target.topic](
          std::shared_ptr<rclcpp::SerializedMessage> message) mutable
        {
          on_message(topic, std::move(message));
        }));
  }
}

}  // namespace rviz_2d_plot_plugin
