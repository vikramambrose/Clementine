/* This file is part of Clementine.
   Copyright 2013, David Sansome <me@davidsansome.com>

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

#include "osdglass.h"

#include <qjson/serializer.h>

#include <QSettings>
#include <QXmlStreamWriter>

#include "core/closure.h"
#include "core/logging.h"
#include "core/network.h"
#include "internet/googleglasssettingspage.h"
#include "internet/oauthenticator.h"

OSDGlass::OSDGlass(QObject* parent)
    : QObject(parent),
      network_(new NetworkAccessManager(this)) {
  QSettings s;
  s.beginGroup(GoogleGlassSettingsPage::kSettingsGroup);

  QString refresh_token = s.value("refresh_token").toString();
  if (refresh_token.isEmpty()) {
    return;
  }

  OAuthenticator* authenticator = new OAuthenticator(
      GoogleGlassSettingsPage::kClientId,
      GoogleGlassSettingsPage::kClientSecret,
      OAuthenticator::RedirectStyle::LOCALHOST,
      this);
  authenticator->RefreshAuthorisation(
      GoogleGlassSettingsPage::kOAuthTokenEndpoint,
      refresh_token);
  NewClosure(authenticator, SIGNAL(Finished()),
      this, SLOT(AuthoriseFinished(OAuthenticator*)), authenticator);
}

void OSDGlass::AuthoriseFinished(OAuthenticator* authenticator) {
  qLog(Debug) << Q_FUNC_INFO << authenticator->access_token();
  access_token_ = authenticator->access_token();
}

void OSDGlass::ShowMessage(
    const QString& summary,
    const QString& message,
    const QString& icon,
    const QImage& image) {
  if (access_token_.isEmpty()) {
    return;
  }

  QString html;
  QXmlStreamWriter writer(&html);
  writer.writeStartElement("article");
    writer.writeStartElement("section");
      writer.writeStartElement("p");
        writer.writeAttribute("class", "text-auto-size");
        writer.writeStartElement("em");
          writer.writeAttribute("class", "yellow");
          writer.writeCharacters(summary);
        writer.writeEndElement();
        writer.writeEmptyElement("br");
        writer.writeStartElement("strong");
          writer.writeAttribute("class", "blue");
          writer.writeCharacters(message);
        writer.writeEndElement();
      writer.writeEndElement();
    writer.writeEndElement();
  writer.writeEndElement();

  QUrl url(GoogleGlassSettingsPage::kGlassTimelineUploadUrl);
  QNetworkRequest request(url);
  request.setRawHeader("Authorization",
      QString("Bearer %1").arg(access_token_).toUtf8());
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QVariantMap data;
  data["html"] = html;
  QJson::Serializer json;
  QNetworkReply* reply = network_->post(request, json.serialize(data));
  NewClosure(reply, SIGNAL(finished()),
      this, SLOT(TimelineInsertFinished(QNetworkReply*)), reply);
}

void OSDGlass::TimelineInsertFinished(QNetworkReply* reply) {
  reply->deleteLater();
  qLog(Debug) << reply->readAll();
}
