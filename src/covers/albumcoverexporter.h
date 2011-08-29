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

#ifndef ALBUMCOVEREXPORTER_H
#define ALBUMCOVEREXPORTER_H

#include "core/song.h"
#include "ui/exportcoversdialog.h"

#include <QHash>
#include <QObject>
#include <QQueue>
#include <QTimer>

// This class represents a single search-for-cover request. It identifies
// and describes the request.
struct CoverExportRequest {
  int id_;
  Song song_;
};

// This class searches for album covers for a given query or artist/album and
// returns URLs. It's NOT thread-safe.
class AlbumCoverExporter : public QObject {
  Q_OBJECT

 public:
  AlbumCoverExporter(const ExportCoversDialog::DialogResult& dialog_result,
                     QObject* parent = 0);
  virtual ~AlbumCoverExporter() {}

  static const int kMaxConcurrentRequests;

  void AddExportRequest(Song song);
  void StartExporting();

  int request_count() { return queued_requests_.size(); }

 signals:
  void AlbumCoversExportUpdate(int exported, int skipped, int all);

 private:
  void CoverExported();
  void CoverSkipped();

  void ProcessAndExportCover(const Song& song);
  void ExportCover(const Song& song);

  void AddRequest(const CoverExportRequest& req);

  ExportCoversDialog::DialogResult dialog_result_;

  QQueue<CoverExportRequest> queued_requests_;
  QHash<quint64, CoverExportRequest> active_requests_;

  QTimer* request_starter_;

  quint64 next_id_;

  int exported_;
  int skipped_;
  int all_;
};

#endif  // ALBUMCOVEREXPORTER_H
