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

#ifndef CLEMENTINE_DATABASE_P_H
#define CLEMENTINE_DATABASE_P_H

#include "core/song.h"
#include "library/directory.h"

#include <clementine/DatabaseDelegate>

class Application;

namespace clementine {

class DatabaseListener;

class DatabasePrivate {
public:
  Application* app_;

  DatabaseListener* listener_;
  QList<DatabaseDelegatePtr> delegates_;
};

class DatabaseListener : public QObject {
  Q_OBJECT

public:
  DatabaseListener(DatabasePrivate* _d, QObject* parent = NULL);

private slots:
  void DirectoryDiscovered(const Directory& dir, const SubdirectoryList& subdirs);
  void DirectoryDeleted(const Directory& dir);

  void SongsDiscovered(const SongList& songs);
  void SongsDeleted(const SongList& songs);
  void TotalSongCountUpdated(int total);

private:
  DatabasePrivate* d;
};

}

#endif // CLEMENTINE_DATABASE_P_H
