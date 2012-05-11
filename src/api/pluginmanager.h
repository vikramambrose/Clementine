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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "availableplugin.h"

#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>

class Application;

namespace clementine {
  class Language;
  class Plugin;
}

class QFileSystemWatcher;
class QTimer;

class PluginManager : public QObject {
  Q_OBJECT

public:
  PluginManager(Application* app);
  ~PluginManager();

  static const char* kSettingsGroup;
  static const char* kIniFileName;
  static const char* kIniSettingsGroup;

  void Init();

  // Get information about the available and loaded plugins.
  QStringList AvailablePlugins() const;
  bool IsPluginLoaded(const QString& id) const;
  clementine::AvailablePlugin PluginInfo(const QString& id) const;

public slots:
  // Load an unload plugins.
  void EnablePlugin(const QString& id);
  void DisablePlugin(const QString& id);

signals:
  void PluginAdded(const QString& id);
  void PluginRemoved(const QString& id);
  void PluginChanged(const QString& id);

private slots:
  void PluginDirectoryChanged();
  void RescanPlugins();

private:
  void LoadSettings();
  void SaveSettings() const;

  void AddLanguage(clementine::Language* language);
  void MaybeAutoEnable(const clementine::AvailablePlugin& plugin);
  QMap<QString, clementine::AvailablePlugin> LoadAllPluginInfo() const;

  bool LoadPluginInfo(const QString& path, clementine::AvailablePlugin*) const;

  bool DoLoadPlugin(const clementine::AvailablePlugin& plugin);

private:
  Application* app_;

  QStringList search_paths_;
  QSet<QString> enabled_plugins_;

  QMap<QString, clementine::Language*> languages_;

  QMap<QString, clementine::AvailablePlugin> available_plugins_;
  QMap<QString, clementine::Plugin*> loaded_plugins_;

  // Watches script directories for changes
  QFileSystemWatcher* watcher_;
  QTimer* rescan_timer_;
};

#endif // PLUGINMANAGER_H
