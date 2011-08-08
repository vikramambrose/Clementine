#include "tinysongprovider.h"

#include <qjson/parser.h>

#include <QNetworkReply>

#include "core/network.h"
#include "songinfotextview.h"

const char* TinySongProvider::kApiKey = "ef686f8dbdc5278558bc2f9a04912ae7";
const char* TinySongProvider::kMetadataUrl = "http://tinysong.com/b/";

TinySongProvider::TinySongProvider()
    : network_(new NetworkAccessManager) {

}

void TinySongProvider::FetchInfo(int id, const Song& metadata) {
  QString base_url(kMetadataUrl);
  base_url += QUrl::toPercentEncoding(metadata.artist());
  base_url += '+';
  base_url += QUrl::toPercentEncoding(metadata.title());

  QUrl url(base_url, QUrl::StrictMode);
  url.addQueryItem("format", "json");
  url.addQueryItem("key", kApiKey);

  QNetworkRequest req(url);
  QNetworkReply* reply = network_->get(req);
  requests_[reply] = id;
  connect(reply, SIGNAL(finished()), SLOT(FetchedInfo()));
}

void TinySongProvider::FetchedInfo() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  Q_ASSERT(reply);
  Q_ASSERT(requests_.contains(reply));

  reply->deleteLater();
  const int id = requests_.take(reply);

  if (reply->error() != QNetworkReply::NoError) {
    emit Finished(id);
    return;
  }

  QJson::Parser parser;
  bool ok = false;
  QVariant result = parser.parse(reply, &ok);

  if (!ok) {
    emit Finished(id);
    return;
  }

  QVariantMap map = result.toMap();
  QString url = map["Url"].toString();


  CollapsibleInfoPane::Data data;
  data.id_ = "tinysong/url";
  data.title_ = "TinySong";
  data.type_ = CollapsibleInfoPane::Data::Type_PlayCounts;

  SongInfoTextView* widget = new SongInfoTextView;
  data.contents_ = widget;
  widget->SetHtml(url);

  emit InfoReady(id, data);
}
