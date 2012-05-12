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

#include "database_p.h"
#include "core/application.h"
#include "core/database.h"
#include "library/librarybackend.h"

#include <clementine/Database>

namespace clementine {

Database::Database(void* app)
  : d(new DatabasePrivate)
{
  d->app_ = reinterpret_cast<Application*>(app);
  d->listener_ = new DatabaseListener(d.get());
}

Database::~Database() {
  delete d->listener_;
}

QString Database::DatabaseUrl() const {
  return "sqlite:///" + d->app_->database()->Connect().databaseName();
}

void Database::RegisterDelegate(DatabaseDelegatePtr delegate) {
  d->delegates_.append(delegate);
}

void Database::UnregisterDelegate(DatabaseDelegatePtr delegate) {
  d->delegates_.removeAll(delegate);
}

void Database::UnregisterAllDelegates() {
  d->delegates_.clear();
}

DatabaseListener::DatabaseListener(DatabasePrivate* _d, QObject* parent)
  : QObject(parent),
    d(_d)
{
  connect(d->app_->library_backend(), SIGNAL(DirectoryDiscovered(Directory,SubdirectoryList)), SLOT(DirectoryDiscovered(Directory,SubdirectoryList)));
  connect(d->app_->library_backend(), SIGNAL(DirectoryDeleted(Directory)), SLOT(DirectoryDeleted(Directory)));
  connect(d->app_->library_backend(), SIGNAL(SongsDiscovered(SongList)), SLOT(SongsDiscovered(SongList)));
  connect(d->app_->library_backend(), SIGNAL(SongsDeleted(SongList)), SLOT(SongsDeleted(SongList)));
  connect(d->app_->library_backend(), SIGNAL(TotalSongCountUpdated(int)), SLOT(TotalSongCountUpdated(int)));
}

void DatabaseListener::DirectoryDiscovered(const Directory& dir, const SubdirectoryList& subdirs) {
  foreach (DatabaseDelegatePtr delegate, d->delegates_) {
    delegate->DirectoryAdded(dir.path);
  }
}

void DatabaseListener::DirectoryDeleted(const Directory& dir) {
  foreach (DatabaseDelegatePtr delegate, d->delegates_) {
    delegate->DirectoryRemoved(dir.path);
  }
}

void DatabaseListener::SongsDiscovered(const SongList& songs) {
  foreach (DatabaseDelegatePtr delegate, d->delegates_) {
    delegate->SongsChanged(songs);
  }
}

void DatabaseListener::SongsDeleted(const SongList& songs) {
  foreach (DatabaseDelegatePtr delegate, d->delegates_) {
    delegate->SongsRemoved(songs);
  }
}

void DatabaseListener::TotalSongCountUpdated(int total) {
  foreach (DatabaseDelegatePtr delegate, d->delegates_) {
    delegate->TotalSongCountUpdated(total);
  }
}

} // namespace clementine
