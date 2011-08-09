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

#include "tinysongprovider.h"

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QNetworkReply>
#include <QTimer>
#include <QXmlStreamWriter>

#include "core/logging.h"
#include "core/mac_startup.h"
#include "core/network.h"
#include "songinfotextview.h"

const char* TinySongProvider::kApiKey = "ef686f8dbdc5278558bc2f9a04912ae7";
const char* TinySongProvider::kMetadataUrl = "http://tinysong.com/b/";
const char* TinySongProvider::kTweetUrl = "https://twitter.com/intent/tweet";

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
  QString title = map["SongName"].toString();

  QString tweet = "From Clementine: " + url;

  QUrl tweet_url(kTweetUrl);
  tweet_url.addQueryItem("text", tweet);

  CollapsibleInfoPane::Data data;
  data.id_ = "tinysong/url";
  data.title_ = "TinySong";
  data.type_ = CollapsibleInfoPane::Data::Type_PlayCounts;

  QString html;
  QXmlStreamWriter xml_writer(&html);
  xml_writer.writeStartElement("a");
  xml_writer.writeAttribute("href", url);
  xml_writer.writeCharacters(title);
  xml_writer.writeEndElement();

  xml_writer.writeStartElement("div");
    xml_writer.writeStartElement("a");
    xml_writer.writeAttribute("href", tweet_url.toString());
      xml_writer.writeStartElement("img");
      xml_writer.writeAttribute("src", ":/tweetn.png");
      xml_writer.writeEndElement();
    xml_writer.writeEndElement();
  xml_writer.writeEndElement();

  xml_writer.writeStartElement("div");
    xml_writer.writeStartElement("a");
    xml_writer.writeAttribute("href", 
      "https://accounts.google.com/o/oauth2/auth"
      "?client_id=679260893280.apps.googleusercontent.com"
      "&redirect_uri=urn:ietf:wg:oauth:2.0:oob"
      "&scope=https://www.google.com/m8/feeds/"
      "&response_type=code");
    xml_writer.writeCharacters("Google OAuth");
    xml_writer.writeEndElement();
  xml_writer.writeEndElement();

  SongInfoTextView* widget = new SongInfoTextView;
  data.contents_ = widget;
  widget->SetHtml(html);

  QTimer* timer = new QTimer();
  timer->setInterval(1000);
  timer->start();
  connect(timer, SIGNAL(timeout()), SLOT(OAuthChecker()));

  emit InfoReady(id, data);
}

void TinySongProvider::OAuthChecker() {
  QString success_code = mac::CheckOAuth();
  if (!success_code.isEmpty()) {
    qLog(Debug) << success_code;
    static_cast<QTimer*>(sender())->stop();
    QString auth_code = success_code.split('=')[1];
    FetchRefreshToken(auth_code);
  }
}

void TinySongProvider::FetchRefreshToken(const QString& auth_code) {
  QUrl url("https://accounts.google.com/o/oauth2/token");
  QString post_data;
  post_data += "client_id=679260893280.apps.googleusercontent.com";
  post_data += "&client_secret=l3cWb8efUZsrBI4wmY3uKl6i";
  post_data += "&code=" + auth_code;
  post_data += "&redirect_uri=urn:ietf:wg:oauth:2.0:oob";
  post_data += "&grant_type=authorization_code";

  QNetworkRequest req(url);
  QNetworkReply* reply = network_->post(req, post_data.toUtf8());
  connect(reply, SIGNAL(finished()), SLOT(FetchRefreshTokenFinished()));
}

void TinySongProvider::FetchRefreshTokenFinished() {
  qLog(Debug) << Q_FUNC_INFO;
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  Q_ASSERT(reply);
  reply->deleteLater();

  QJson::Parser parser;
  bool ok = false;
  QVariant result = parser.parse(reply, &ok);
  qLog(Debug) << result;
  if (!ok) {
    qLog(Warning) << "Could not parse json from Google oauth";
    return;
  }

  QVariantMap map = result.toMap();
  QString access_token = map["access_token"].toString();
  qLog(Debug) << access_token;
}
