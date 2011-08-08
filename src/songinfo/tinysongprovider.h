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

#ifndef TINYSONGPROVIDER_H_
#define TINYSONGPROVIDER_H_

#include "core/song.h"
#include "songinfoprovider.h"

#include <QMap>

class QNetworkReply;
class NetworkAccessManager;

class TinySongProvider : public SongInfoProvider {
  Q_OBJECT

 public:
  TinySongProvider();
  virtual ~TinySongProvider() {}

  virtual void FetchInfo(int id, const Song& metadata);

 private slots:
  void FetchedInfo();

 private:
  QMap<QNetworkReply*, int> requests_;
  NetworkAccessManager* network_;

  static const char* kApiKey;
  static const char* kMetadataUrl;
  static const char* kTweetUrl;
};

#endif
