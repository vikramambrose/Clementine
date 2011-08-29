/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>

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

#include "albumcoverexporter.h"
#include "core/song.h"

#include <QFile>

const int AlbumCoverExporter::kMaxConcurrentRequests = 5;

AlbumCoverExporter::AlbumCoverExporter(const ExportCoversDialog::DialogResult& dialog_result,
                                       QObject* parent)
    : QObject(parent),
      dialog_result_(dialog_result),
      exported_(0),
      skipped_(0),
      all_(0)
{
}

void AlbumCoverExporter::AddExportRequest(Song song) {
  CoverExportRequest request;
  request.id_ = next_id_++;
  request.song_ = song;

  queued_requests_.enqueue(request);

  all_++;
}

void AlbumCoverExporter::StartExporting() {
  while (!queued_requests_.isEmpty()) {

    CoverExportRequest request = queued_requests_.dequeue();

    active_requests_[request.id_] = request;

    QString cover_path = request.song_.CoverPath();

    // manually unset?
    if(cover_path.isEmpty()) {
      CoverSkipped();
      continue;

    } else {
      if(dialog_result_.RequiresCoverProcessing()) {
        ProcessAndExportCover(request.song_);
      } else {
        ExportCover(request.song_);
      }

    }

    active_requests_.remove(request.id_);
  }
}

// Exports a single album cover using a "save QImage to file" approach.
// For performance reasons this method will be invoked only if loading
// and in memory processing of images is necessary for current settings
// which means that:
// - either the force size flag is being used
// - or the "overwrite smaller" mode is used
// In all other cases, the faster ExportCover() method will be used.
void AlbumCoverExporter::ProcessAndExportCover(const Song& song) {
  QString cover_path = song.CoverPath();

  // either embedded or disk - the one we'll export for the current album
  QImage cover;

  QImage embedded_cover;
  QImage disk_cover;

  // load embedded cover if any
  if(song.has_embedded_cover()) {
    embedded_cover = Song::LoadEmbeddedArt(song.url().toLocalFile());

    if(embedded_cover.isNull()) {
      CoverSkipped();
      return;
    }

    cover = embedded_cover;

  }

  // load a file cover which iss mandatory if there's no embedded cover
  disk_cover.load(cover_path);

  if(embedded_cover.isNull()) {
    if(disk_cover.isNull()) {
      CoverSkipped();
      return;
    }

    cover = disk_cover;

  }

  // rescale if necessary
  if(dialog_result_.IsSizeForced()) {
    cover = cover.scaled(QSize(dialog_result_.width_, dialog_result_.height_), Qt::IgnoreAspectRatio);
  }

  QString dir = song.url().toLocalFile().section('/', 0, -2);
  QString extension = cover_path.section('.', -1);

  QString new_file = dir + '/' + dialog_result_.fileName_ + '.' +
                         (cover_path == Song::kEmbeddedCover
                            ? "jpg"
                            : extension);

  // we're handling overwrite as remove + copy so we need to delete the old file first
  if(QFile::exists(new_file) && dialog_result_.overwrite_ != ExportCoversDialog::OverwriteMode_None) {

    // if the mode is "overwrite smaller" then skip the cover if a bigger one
    // is already available in the folder
    if(dialog_result_.overwrite_ == ExportCoversDialog::OverwriteMode_Smaller) {
      QImage existing;
      existing.load(new_file);

      if(existing.isNull() || existing.size().height() >= cover.size().height()
           || existing.size().width() >= cover.size().width()) {

        CoverSkipped();
        return;

      }
    }

    if(!QFile::remove(new_file)) {
      CoverSkipped();
      return;
    }
  }

  if(cover.save(new_file)) {
    CoverExported();
  } else {
    CoverSkipped();
  }
}

// Exports a single album cover using a "copy file" approach.
void AlbumCoverExporter::ExportCover(const Song& song) {
  QString cover_path = song.CoverPath();

  QString dir = song.url().toLocalFile().section('/', 0, -2);
  QString extension = cover_path.section('.', -1);

  QString new_file = dir + '/' + dialog_result_.fileName_ + '.' +
                         (cover_path == Song::kEmbeddedCover
                            ? "jpg"
                            : extension);

  // we're handling overwrite as remove + copy so we need to delete the old file first
  if(dialog_result_.overwrite_ != ExportCoversDialog::OverwriteMode_None && QFile::exists(new_file)) {
    if(!QFile::remove(new_file)) {
      CoverSkipped();
      return;
    }
  }

  // an embedded cover
  if(cover_path == Song::kEmbeddedCover) {

    QImage embedded = Song::LoadEmbeddedArt(song.url().toLocalFile());
    if(!embedded.save(new_file)) {
      CoverSkipped();
      return;
    }

 // automatic or manual cover, available in an image file
  } else {

    if(!QFile::copy(cover_path, new_file)) {
      CoverSkipped();
      return;
    }

  }

  CoverExported();
}

void AlbumCoverExporter::CoverExported() {
  exported_++;
  emit AlbumCoversExportUpdate(exported_, skipped_, all_);
}


void AlbumCoverExporter::CoverSkipped() {
  skipped_++;
  emit AlbumCoversExportUpdate(exported_, skipped_, all_);
}
