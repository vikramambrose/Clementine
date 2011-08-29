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
#include "coverexportrunnable.h"
#include "core/song.h"

#include <QFile>
#include <QThreadPool>

const int AlbumCoverExporter::kMaxConcurrentRequests = 5;

AlbumCoverExporter::AlbumCoverExporter(const ExportCoversDialog::DialogResult& dialog_result,
                                       QObject* parent)
    : QObject(parent),
      dialog_result_(dialog_result),
      thread_pool_(new QThreadPool(this)),
      exported_(0),
      skipped_(0),
      all_(0)
{
  thread_pool_->setMaxThreadCount(kMaxConcurrentRequests);
}

void AlbumCoverExporter::AddExportRequest(Song song) {
  requests_.append(new CoverExportRunnable(dialog_result_, song));
  all_++;
}

void AlbumCoverExporter::StartExporting() {
  while (!requests_.isEmpty()) {
    CoverExportRunnable* runnable = requests_.dequeue();

    connect(runnable, SIGNAL(CoverExported()), SLOT(CoverExported()));
    connect(runnable, SIGNAL(CoverSkipped()), SLOT(CoverSkipped()));

    thread_pool_->start(runnable);
  }

  thread_pool_->waitForDone();
}

void AlbumCoverExporter::CoverExported() {
  exported_++;
  emit AlbumCoversExportUpdate(exported_, skipped_, all_);
}

void AlbumCoverExporter::CoverSkipped() {
  skipped_++;
  emit AlbumCoversExportUpdate(exported_, skipped_, all_);
}
