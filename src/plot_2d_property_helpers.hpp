// Copyright 2026 Abdelrahman Mahmoud
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef PLOT_2D_PROPERTY_HELPERS_HPP_
#define PLOT_2D_PROPERTY_HELPERS_HPP_

#include <QComboBox>
#include <QCompleter>
#include <QStyleOptionViewItem>
#include <QVariant>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <rviz_common/properties/bool_property.hpp>
#include <rviz_common/properties/editable_enum_property.hpp>
#include <rviz_common/properties/property.hpp>
#include <rviz_common/properties/property_tree_model.hpp>

namespace rviz_2d_plot_plugin
{

class ContainsFilterEditableEnumProperty
  : public rviz_common::properties::EditableEnumProperty
{
public:
  using rviz_common::properties::EditableEnumProperty::EditableEnumProperty;

  QWidget * createEditor(
    QWidget * parent,
    const QStyleOptionViewItem & option) override
  {
    QWidget * editor =
      rviz_common::properties::EditableEnumProperty::createEditor(parent, option);
    auto * combo_box = qobject_cast<QComboBox *>(editor);
    if (combo_box && combo_box->completer()) {
      combo_box->completer()->setCompletionMode(QCompleter::PopupCompletion);
      combo_box->completer()->setCaseSensitivity(Qt::CaseInsensitive);
      combo_box->completer()->setFilterMode(Qt::MatchContains);
    }
    return editor;
  }
};

class ListItemBoolProperty : public rviz_common::properties::BoolProperty
{
public:
  using rviz_common::properties::BoolProperty::BoolProperty;

  void setDisplayLabel(const QString & label)
  {
    if (display_label_ == label) {
      return;
    }
    display_label_ = label;
    if (model_) {
      model_->emitDataChanged(this);
    }
  }

  QVariant getViewData(const int column, const int role) const override
  {
    if (column == 0 && role == Qt::DisplayRole && !display_label_.isEmpty()) {
      return display_label_;
    }
    return rviz_common::properties::BoolProperty::getViewData(column, role);
  }

  Qt::ItemFlags getViewFlags(const int column) const override
  {
    Qt::ItemFlags flags = rviz_common::properties::BoolProperty::getViewFlags(column);
    if (column == 0) {
      flags |= Qt::ItemIsDragEnabled;
    }
    return flags;
  }

private:
  QString display_label_;
};

class ReorderableListProperty : public rviz_common::properties::Property
{
public:
  using rviz_common::properties::Property::Property;

  void setFixedChildCount(const int count)
  {
    fixed_child_count_ = std::max(0, count);
  }

  Qt::ItemFlags getViewFlags(const int column) const override
  {
    Qt::ItemFlags flags = rviz_common::properties::Property::getViewFlags(column);
    if (column == 0) {
      flags |= Qt::ItemIsDropEnabled;
    }
    return flags;
  }

  void addChild(rviz_common::properties::Property * child, int index = -1) override
  {
    if (dynamic_cast<ListItemBoolProperty *>(child) && index >= 0) {
      index = std::max(fixed_child_count_, index);
    }
    rviz_common::properties::Property::addChild(child, index);
  }

private:
  int fixed_child_count_{0};
};

template<typename PropertySet>
bool syncPropertyOrder(
  rviz_common::properties::Property * root,
  const int fixed_child_count,
  std::vector<PropertySet> & properties,
  const QString & row_name_prefix)
{
  if (!root) {
    return false;
  }

  std::vector<PropertySet> ordered;
  ordered.reserve(properties.size());
  for (int i = fixed_child_count; i < root->numChildren(); ++i) {
    rviz_common::properties::Property * child = root->childAt(i);
    const auto it = std::find_if(
      properties.begin(), properties.end(),
      [child](const PropertySet & item) {
        return item.root == child;
      });
    if (it != properties.end()) {
      ordered.push_back(*it);
    }
  }

  if (ordered.size() != properties.size()) {
    return false;
  }

  bool changed = false;
  for (std::size_t i = 0; i < ordered.size(); ++i) {
    changed = changed || ordered[i].root != properties[i].root;
  }
  if (!changed) {
    return false;
  }

  properties = std::move(ordered);
  for (std::size_t i = 0; i < properties.size(); ++i) {
    if (properties[i].root) {
      properties[i].root->setName(row_name_prefix + QString::number(static_cast<int>(i) + 1));
    }
  }
  return true;
}

}  // namespace rviz_2d_plot_plugin

#endif  // PLOT_2D_PROPERTY_HELPERS_HPP_
