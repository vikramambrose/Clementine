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

#ifndef PLAYER_P_H
#define PLAYER_P_H

#include <clementine/PlayerDelegate>

#include <QObject>

class Application;

namespace clementine {

class PlayerListener;

struct PlayerPrivate {
  Application* app_;

  PlayerListener* listener_;
  QList<PlayerDelegatePtr> delegates_;
};

class PlayerListener : public QObject {
  Q_OBJECT

public:
  PlayerListener(PlayerPrivate* _d, QObject* parent = NULL);

public slots:
  void Playing();
  void Paused();
  void Stopped();
  void PlaylistFinished();
  void VolumeChanged(int volume);
  void Seeked(qlonglong microseconds);

private:
  PlayerPrivate* d;
};

}

#endif // PLAYER_P_H
