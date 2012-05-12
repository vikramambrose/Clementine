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

#include "action_p.h"
#include "core/logging.h"

#include <clementine/Action>

#include <QAction>

namespace clementine {

Action::Action(const QString& text)
  : d(new ActionPrivate(this))
{
  d->text_ = text;
}

Action::Action(const QString& icon, const QString& text)
  : d(new ActionPrivate(this))
{
  d->icon_ = icon;
  d->text_ = text;
}

Action::~Action() {
  Unattach();
}

bool Action::Attach(void* data) {
  if (d->q_action_) {
    return false;
  }

  d->q_action_ = reinterpret_cast<QAction*>(data);
  d->q_action_->setText(d->text_);
  d->q_action_->setEnabled(d->enabled_);

  d->listener_ = new ActionListener(d.get());

  // TODO: Resolve the icon from the plugin's directory.

  return true;
}

void Action::Unattach() {
  delete d->listener_; d->listener_ = NULL;
  delete d->q_action_; d->q_action_ = NULL;
}

bool Action::IsEnabled() const {
  return d->enabled_;
}

QString Action::Text() const {
  return d->text_;
}

QString Action::Icon() const {
  return d->icon_;
}

void Action::SetEnabled(bool enabled) {
  d->enabled_ = enabled;
  if (d->q_action_) {
    d->q_action_->setEnabled(enabled);
  }
}

void Action::SetText(const QString& text) {
  d->text_ = text;
  if (d->q_action_) {
    d->q_action_->setText(text);
  }
}

void Action::SetIcon(const QString& icon) {
  d->icon_ = icon;
  // TODO
}

void Action::Triggered() {
}

ActionListener::ActionListener(ActionPrivate* _d, QObject* parent)
  : QObject(parent),
    d(_d)
{
  connect(d->q_action_, SIGNAL(triggered()), SLOT(Triggered()));
}

void ActionListener::Triggered() {
  d->action_->Triggered();
}

} // namespace clementine
