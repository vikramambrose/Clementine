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
};

#endif
