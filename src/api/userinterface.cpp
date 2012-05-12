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
#include "core/application.h"
#include "core/logging.h"

#include <clementine/Action>
#include <clementine/UserInterface>

#include <QAction>

namespace clementine {

struct UserInterface::Private {
  Application* app_;

  QList<ActionPtr> actions_;
};

UserInterface::UserInterface(void* app)
  : d(new Private)
{
  d->app_ = reinterpret_cast<Application*>(app);
}

UserInterface::~UserInterface() {
}

bool UserInterface::AddMenuItem(const QString& location, ActionPtr action) {
  // Create a QAction for this Action.
  QAction* q_action = new QAction(NULL);
  if (!action->Attach(q_action)) {
    qLog(Warning) << "Actions can't be added to more than one location";
    delete q_action;
    return false;
  }

  // Add it to the right place in the menu.
  if (!d->app_->plugin_manager()->AddAction(location, q_action)) {
    qLog(Warning) << "Unknown Action location" << location;
    // If it couldn't be added, unattach it immediately.
    action->Unattach();
    return false;
  }

  d->actions_.append(action);

  return true;
}

void UserInterface::RemoveMenuItem(ActionPtr action) {
  d->actions_.removeAll(action);
  action->Unattach();
}

void UserInterface::RemoveAll() {
  d->actions_.clear();
}

} // namespace clementine
