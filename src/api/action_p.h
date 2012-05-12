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

#ifndef CLEMENTINE_ACTION_P_H
#define CLEMENTINE_ACTION_P_H

#include <QObject>

class QAction;

namespace clementine {

class Action;
class ActionListener;

class ActionPrivate {
public:
  ActionPrivate(Action* action)
    : action_(action),
      listener_(NULL),
      enabled_(true),
      q_action_(NULL)
  {}

  Action* action_;
  ActionListener* listener_;

  bool enabled_;
  QString text_;
  QString icon_;

  QAction* q_action_;
};

class ActionListener : public QObject {
  Q_OBJECT

public:
  ActionListener(ActionPrivate* _d, QObject* parent = NULL);

public slots:
  void Triggered();

private:
  ActionPrivate* d;
};

}

#endif // CLEMENTINE_ACTION_P_H
