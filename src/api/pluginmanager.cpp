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

#include "availableplugin.h"
#include "config.h"
#include "language.h"
#include "pluginmanager.h"
#include "core/logging.h"
#include "core/utilities.h"

#ifdef HAVE_PYTHON
#  include "pythonlanguage.h"
#endif

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QSettings>
#include <QTimer>

#include <clementine/Clementine>
#include <clementine/Plugin>

const char* PluginManager::kSettingsGroup = "Plugins";
const char* PluginManager::kIniFileName = "clementine-plugin.ini";
const char* PluginManager::kIniSettingsGroup = "ClementinePlugin";

PluginManager::PluginManager(Application* app)
  : app_(app),
    watcher_(new QFileSystemWatcher(this)),
    rescan_timer_(new QTimer(this))
{
  // Create languages
#ifdef HAVE_PYTHON
  AddLanguage(new PythonLanguage(app));
#endif

  connect(watcher_, SIGNAL(directoryChanged(QString)), SLOT(PluginDirectoryChanged()));

  rescan_timer_->setSingleShot(true);
  rescan_timer_->setInterval(1000);
  connect(rescan_timer_, SIGNAL(timeout()), SLOT(RescanPlugins()));

  // Create the user's plugins directory if it doesn't exist yet
  QString local_path = Utilities::GetConfigPath(Utilities::Path_Plugins);
  if (!QFile::exists(local_path)) {
    if (!QDir().mkpath(local_path)) {
      qLog(Warning) << "Couldn't create directory" << local_path;
    }
  }

  search_paths_
      << local_path
#ifdef USE_INSTALL_PREFIX
      << CMAKE_INSTALL_PREFIX "/share/clementine/plugins"
#endif
      << "/usr/share/clementine/plugins"
      << "/usr/local/share/clementine/plugins";

#if defined(Q_OS_WIN32)
  search_paths_ << QCoreApplication::applicationDirPath() + "/plugins";
#elif defined(Q_OS_MAC)
  search_paths_ << mac::GetResourcesPath() + "/plugins";
#endif

  qLog(Debug) << "Plugin search paths:" << search_paths_;
}

PluginManager::~PluginManager() {
}

void PluginManager::AddLanguage(clementine::Language* language) {
  languages_[language->name()] = language;
}

void PluginManager::Init() {
  // Load settings
  LoadSettings();

  // Get the available plugins.
  available_plugins_ = LoadAllPluginInfo();

  // Enable the ones that were enabled last time.
  foreach (const clementine::AvailablePlugin& info, available_plugins_) {
    MaybeAutoEnable(info);

    emit PluginAdded(info.id_);
  }
}

void PluginManager::LoadSettings() {
  QSettings s;
  s.beginGroup(kSettingsGroup);
  enabled_plugins_ = QSet<QString>::fromList(
      s.value("enabled_plugins").toStringList());
}

void PluginManager::SaveSettings() const {
  QSettings s;
  s.beginGroup(kSettingsGroup);
  s.setValue("enabled_plugins",
      QVariant::fromValue<QStringList>(enabled_plugins_.toList()));
}

void PluginManager::MaybeAutoEnable(const clementine::AvailablePlugin& info) {
  if (loaded_plugins_.contains(info.id_)) {
    // Already loaded
    return;
  }

  // Load the plugin if it's enabled
  //if (enabled_plugins_.contains(info.id_)) {
    if (!DoLoadPlugin(info)) {
      // Failed to load?  Disable it so we don't try again
      enabled_plugins_.remove(info.id_);
      SaveSettings();
    }
  //}
}

QMap<QString, clementine::AvailablePlugin> PluginManager::LoadAllPluginInfo() const {
  QMap<QString, clementine::AvailablePlugin> ret;

  foreach (const QString& search_path, search_paths_) {
    if (!QFile::exists(search_path))
      continue;

    if (!watcher_->directories().contains(search_path)) {
      qLog(Debug) << "Adding directory watch:" << search_path;
      watcher_->addPath(search_path);
    }

    QDirIterator it(search_path,
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable | QDir::Executable,
        QDirIterator::FollowSymlinks);
    while (it.hasNext()) {
      it.next();
      const QString path = it.filePath();
      if (!watcher_->directories().contains(path)) {
        qLog(Debug) << "Adding directory watch:" << path;
        watcher_->addPath(path);
      }

      clementine::AvailablePlugin info;
      if (!LoadPluginInfo(path, &info)) {
        qLog(Warning) << "Not a valid Clementine plugin directory, ignoring:"
                      << path;
        continue;
      }

      if (ret.contains(info.id_)) {
        // Seen this plugin already
        continue;
      }

      ret.insert(info.id_, info);
    }
  }

  return ret;
}

void PluginManager::PluginDirectoryChanged() {
  rescan_timer_->start();
}

void PluginManager::RescanPlugins() {
  // Get the new list of plugins
  QMap<QString, clementine::AvailablePlugin> new_info = LoadAllPluginInfo();

  // Look at existing plugins, find ones that have changed or been deleted
/*  for (int i=0 ; i<available_plugins_.count() ; ++i) {
    ScriptInfo* info = &info_[i];
    const QString id = info->id();

    if (!new_info.contains(id)) {
      // This script was deleted - unload it and remove it from the model
      Disable(index(i));

      beginRemoveRows(QModelIndex(), i, i);
      info_.removeAt(i);
      endRemoveRows();

      --i;
      continue;
    }

    if (*info != new_info[id]) {
      // This script was modified - change the metadata in the existing entry
      info->TakeMetadataFrom(new_info[id]);

      emit dataChanged(index(i), index(i));

      AddLogLine("Watcher",
        tr("The '%1' script was modified, you might have to reload it").arg(id), false);
    }

    new_info.remove(id);
  }

  // Things that are left in new_info are newly added scripts
  if (!new_info.isEmpty()) {
    const int begin = info_.count();
    const int end = begin + new_info.count() - 1;

    beginInsertRows(QModelIndex(), begin, end);
    info_.append(new_info.values());

    for (int i=begin ; i<=end ; ++i) {
      MaybeAutoEnable(&info_[i]);
    }

    endInsertRows();
  }*/
}

bool PluginManager::LoadPluginInfo(const QString& path,
                                clementine::AvailablePlugin* info) const {
  const QString ini_file = path + "/" + kIniFileName;
  const QString id = QFileInfo(path).fileName();

  // Does the file exist?
  if (!QFile::exists(ini_file)) {
    qLog(Warning) << "Plugin definition file not found:" << ini_file;
    return false;
  }

  qLog(Debug) << "Reading plugin:" << ini_file;

  // Open it
  QSettings s(ini_file, QSettings::IniFormat);
  if (!s.childGroups().contains(kIniSettingsGroup)) {
    qLog(Warning) << "Missing" << kIniSettingsGroup << "section in" << ini_file;
    return false;
  }
  s.beginGroup(kIniSettingsGroup);

  // Find out what language it's in
  QString language_name = s.value("language").toString();
  clementine::Language* language = languages_[language_name];
  if (!language) {
    qLog(Warning) << "Unknown language" << language_name << "in" << ini_file;
    return false;
  }

  info->language_ = language;

  // Load the rest of the metadata
  info->path_ = path;
  info->id_ = id;
  info->name_ = s.value("name").toString();
  info->description_ = s.value("description").toString();
  info->icon_filename_ = QFileInfo(QDir(path), s.value("icon").toString()).absoluteFilePath();

  // Replace special characters in the ID
  info->id_.replace(QRegExp("[^a-zA-Z0-9_]"), "_");

  return true;
}

bool PluginManager::IsPluginLoaded(const QString& id) const {
  return loaded_plugins_.contains(id);
}

clementine::AvailablePlugin PluginManager::PluginInfo(const QString& id) const {
  return available_plugins_[id];
}

QStringList PluginManager::AvailablePlugins() const {
  return available_plugins_.keys();
}

void PluginManager::EnablePlugin(const QString& id) {
  if (loaded_plugins_.contains(id)) {
    // Already loaded.
    return;
  }

  if (DoLoadPlugin(available_plugins_[id])) {
    enabled_plugins_.insert(id);
    SaveSettings();

    emit PluginChanged(id);
  }
}

bool PluginManager::DoLoadPlugin(const clementine::AvailablePlugin& info) {
  qLog(Debug) << "Loading plugin" << info.id_;

  info.language_->EnsureInitialised();
  clementine::Plugin* plugin = info.language_->LoadPlugin(info);
  if (plugin) {
    loaded_plugins_[info.id_] = plugin;
  }

  return plugin;
}

void PluginManager::DisablePlugin(const QString& id) {
  if (!loaded_plugins_.contains(id)) {
    // Not loaded.
    return;
  }

  qLog(Debug) << "Unloading plugin" << id;

  clementine::Plugin* plugin = loaded_plugins_[id];
  plugin->plugin_info().language_->UnloadPlugin(plugin);

  loaded_plugins_.remove(id);
  enabled_plugins_.remove(id);
  SaveSettings();

  emit PluginChanged(id);
}

void PluginManager::RegisterExtensionPoint(const QString& name, QMenu* menu, QAction* before) {
  Q_ASSERT(!extension_points_.contains(name));

  extension_points_[name] = ExtensionPoint(menu, before);
}

bool PluginManager::AddAction(const QString& extension_point, QAction* action) {
  if (!extension_points_.contains(extension_point))
    return false;

  const ExtensionPoint& point = extension_points_[extension_point];
  point.menu_->insertAction(point.before_, action);

  return true;
}
