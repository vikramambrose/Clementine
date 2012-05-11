/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>
   
   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pluginmanager.h"
#include "pluginmodel.h"
#include "language.h"

PluginModel::PluginModel(PluginManager* manager, QObject* parent)
  : QStandardItemModel(parent),
    manager_(manager)
{
  connect(manager_, SIGNAL(PluginAdded(QString)), SLOT(PluginAdded(QString)));
  connect(manager_, SIGNAL(PluginRemoved(QString)), SLOT(PluginRemoved(QString)));
  connect(manager_, SIGNAL(PluginChanged(QString)), SLOT(PluginChanged(QString)));

  foreach (const QString& id, manager->AvailablePlugins()) {
    PluginAdded(id);
  }
}

void PluginModel::PluginAdded(const QString& id) {
  QStandardItem* item = new QStandardItem;

  items_[id] = item;
  PluginChanged(id);

  appendRow(item);
}

void PluginModel::PluginRemoved(const QString& id) {

}

void PluginModel::PluginChanged(const QString& id) {
  QStandardItem* item = items_[id];
  const clementine::AvailablePlugin& info = manager_->PluginInfo(id);

  item->setText(info.name_);
  item->setData(info.description_, Role_Description);
  item->setData(info.language_->name(), Role_Language);
  item->setData(info.id_, Role_Id);
  item->setData(manager_->IsPluginLoaded(id), Role_IsEnabled);
}

void PluginModel::Enable(const QModelIndex& index) {
  manager_->EnablePlugin(index.data(Role_Id).toString());
}

void PluginModel::Disable(const QModelIndex& index) {
  manager_->DisablePlugin(index.data(Role_Id).toString());
}
