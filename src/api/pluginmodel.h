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

#ifndef PLUGINMODEL_H
#define PLUGINMODEL_H

#include <QStandardItemModel>

class PluginManager;

class PluginModel : public QStandardItemModel {
  Q_OBJECT

public:
  PluginModel(PluginManager* manager, QObject* parent = 0);

  enum Role {
    Role_Description = Qt::UserRole,
    Role_Language,
    Role_IsEnabled,
    Role_Id,

    RoleCount
  };

public slots:
  void Enable(const QModelIndex& index);
  void Disable(const QModelIndex& index);

private slots:
  void PluginAdded(const QString& id);
  void PluginRemoved(const QString& id);
  void PluginChanged(const QString& id);

private:
  PluginManager* manager_;
  QMap<QString, QStandardItem*> items_;
};

#endif // PLUGINMODEL_H
