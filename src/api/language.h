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

#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <QString>

class Application;

namespace clementine {

class AvailablePlugin;
class Clementine;
class Language;
class Plugin;

// Each Language finds and loads its own plugins.
class Language {
public:
  Language(Application* app);
  virtual ~Language() {}

  virtual QString name() const = 0;

  // Loads a plugin that hasn't been loaded yet.  Returns the new plugin, or
  // NULL if it couldn't be loaded.
  virtual Plugin* LoadPlugin(const AvailablePlugin& plugin) = 0;

  // Unloads an already loaded plugin.  This must also delete the plugin object.
  virtual void UnloadPlugin(Plugin* plugin) = 0;

  bool EnsureInitialised();

protected:
  // Called before this Language's first plugin is loaded.
  virtual bool Init() = 0;

  Application* app() const { return app_; }

private:
  Application* app_;
  bool is_initialised_;
};

} // namespace clementine

#endif // LANGUAGE_H
