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

CoverExportRunnable::CoverExportRunnable(const ExportCoversDialog::DialogResult& dialog_result,
                                         const Song& song)
    : dialog_result_(dialog_result),
      song_(song)
{
}

void CoverExportRunnable::run() {
  QString cover_path = song_.CoverPath();

  // manually unset?
  if(cover_path.isEmpty()) {
    EmitCoverSkipped();

  } else {
    if(dialog_result_.RequiresCoverProcessing()) {
      ProcessAndExportCover();
    } else {
      ExportCover();
    }

  }
}

// Exports a single album cover using a "save QImage to file" approach.
// For performance reasons this method will be invoked only if loading
// and in memory processing of images is necessary for current settings
// which means that:
// - either the force size flag is being used
// - or the "overwrite smaller" mode is used
// In all other cases, the faster ExportCover() method will be used.
void CoverExportRunnable::ProcessAndExportCover() {
  QString cover_path = song_.CoverPath();

  // either embedded or disk - the one we'll export for the current album
  QImage cover;

  QImage embedded_cover;
  QImage disk_cover;

  // load embedded cover if any
  if(song_.has_embedded_cover()) {
    embedded_cover = Song::LoadEmbeddedArt(song_.url().toLocalFile());

    if(embedded_cover.isNull()) {
      EmitCoverSkipped();
      return;
    }

    cover = embedded_cover;

  }

  // load a file cover which iss mandatory if there's no embedded cover
  disk_cover.load(cover_path);

  if(embedded_cover.isNull()) {
    if(disk_cover.isNull()) {
      EmitCoverSkipped();
      return;
    }

    cover = disk_cover;

  }

  // rescale if necessary
  if(dialog_result_.IsSizeForced()) {
    cover = cover.scaled(QSize(dialog_result_.width_, dialog_result_.height_), Qt::IgnoreAspectRatio);
  }

  QString dir = song_.url().toLocalFile().section('/', 0, -2);
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

        EmitCoverSkipped();
        return;

      }
    }

    if(!QFile::remove(new_file)) {
      EmitCoverSkipped();
      return;
    }
  }

  if(cover.save(new_file)) {
    EmitCoverExported();
  } else {
    EmitCoverSkipped();
  }
}

// Exports a single album cover using a "copy file" approach.
void CoverExportRunnable::ExportCover() {
  QString cover_path = song_.CoverPath();

  QString dir = song_.url().toLocalFile().section('/', 0, -2);
  QString extension = cover_path.section('.', -1);

  QString new_file = dir + '/' + dialog_result_.fileName_ + '.' +
                         (cover_path == Song::kEmbeddedCover
                            ? "jpg"
                            : extension);

  // we're handling overwrite as remove + copy so we need to delete the old file first
  if(dialog_result_.overwrite_ != ExportCoversDialog::OverwriteMode_None && QFile::exists(new_file)) {
    if(!QFile::remove(new_file)) {
      EmitCoverSkipped();
      return;
    }
  }

  // an embedded cover
  if(cover_path == Song::kEmbeddedCover) {

    QImage embedded = Song::LoadEmbeddedArt(song_.url().toLocalFile());
    if(!embedded.save(new_file)) {
      EmitCoverSkipped();
      return;
    }

 // automatic or manual cover, available in an image file
  } else {

    if(!QFile::copy(cover_path, new_file)) {
      EmitCoverSkipped();
      return;
    }

  }

  EmitCoverExported();
}

void CoverExportRunnable::EmitCoverExported() {
  emit CoverExported();
}


void CoverExportRunnable::EmitCoverSkipped() {
  emit CoverSkipped();
}
