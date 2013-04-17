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

#include "googleglasssettingspage.h"
#include "ui_googleglasssettingspage.h"

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include "core/application.h"
#include "core/closure.h"
#include "core/logging.h"
#include "core/network.h"
#include "internet/oauthenticator.h"
#include "ui/settingsdialog.h"

namespace {

static const char* kSettingsGroup = "GoogleGlass";
static const char* kOAuthEndpoint = "https://accounts.google.com/o/oauth2/auth";
static const char* kOAuthTokenEndpoint =
    "https://accounts.google.com/o/oauth2/token";
static const char* kOAuthScope =
    "https://www.googleapis.com/auth/userinfo.email"
    " https://www.googleapis.com/auth/glass.timeline";
static const char* kClientId =
    "701906459320.apps.googleusercontent.com";
static const char* kClientSecret = "oKx6S1Bl0TwVeXv9XRswJGCN";
static const char* kGoogleUserInfoEndpoint =
    "https://www.googleapis.com/oauth2/v1/userinfo";

static const char* kGlassTimelineUploadUrl =
    "https://www.googleapis.com/mirror/v1/timeline";

}  // namespace

GoogleGlassSettingsPage::GoogleGlassSettingsPage(SettingsDialog* parent)
  : SettingsPage(parent),
    ui_(new Ui::GoogleGlassSettingsPage),
    network_(new NetworkAccessManager(this)) {
  ui_->setupUi(this);
  ui_->login_state->AddCredentialGroup(ui_->login_container);

  connect(ui_->login_button, SIGNAL(clicked()), SLOT(LoginClicked()));
  connect(ui_->login_state, SIGNAL(LogoutClicked()), SLOT(LogoutClicked()));

  dialog()->installEventFilter(this);
}

GoogleGlassSettingsPage::~GoogleGlassSettingsPage() {
  delete ui_;
}

void GoogleGlassSettingsPage::Load() {
  QSettings s;
  s.beginGroup(kSettingsGroup);

  const QString user_email = s.value("user_email").toString();
  const QString refresh_token = s.value("refresh_token").toString();

  if (!user_email.isEmpty() && !refresh_token.isEmpty()) {
    ui_->login_state->SetLoggedIn(LoginStateWidget::LoggedIn, user_email);
  }
}

void GoogleGlassSettingsPage::Save() {
}

void GoogleGlassSettingsPage::LoginClicked() {
  qLog(Debug) << Q_FUNC_INFO;
  ui_->login_button->setEnabled(false);
  OAuthenticator* authenticator = new OAuthenticator(
      kClientId,
      kClientSecret,
      OAuthenticator::RedirectStyle::LOCALHOST,
      this);
  authenticator->StartAuthorisation(
      kOAuthEndpoint,
      kOAuthTokenEndpoint,
      kOAuthScope);
  NewClosure(authenticator, SIGNAL(Finished()),
      this, SLOT(AuthoriseFinished(OAuthenticator*)), authenticator);
}

void GoogleGlassSettingsPage::AuthoriseFinished(OAuthenticator* authenticator) {
  qLog(Debug) << Q_FUNC_INFO;
  authenticator->deleteLater();
  access_token_ = authenticator->access_token();

  QSettings s;
  s.beginGroup(kSettingsGroup);
  s.setValue("refresh_token", authenticator->refresh_token());

  QUrl url(kGoogleUserInfoEndpoint);
  QNetworkRequest request(url);
  request.setRawHeader("Authorization",
      QString("Bearer %1").arg(access_token_).toUtf8());
  qLog(Debug) << "Requesting user info";
  QNetworkReply* reply = network_->get(request);
  NewClosure(reply, SIGNAL(finished()),
      this, SLOT(FetchUserInfoFinished(QNetworkReply*)), reply);
}

void GoogleGlassSettingsPage::FetchUserInfoFinished(QNetworkReply* reply) {
  qLog(Debug) << Q_FUNC_INFO;
  reply->deleteLater();
  QJson::Parser parser;
  bool ok = false;
  QVariantMap result = parser.parse(reply, &ok).toMap();
  qLog(Debug) << result;
  if (!ok) {
    qLog(Error) << "Failed to parse user info reply";
    return;
  }

  QString user_email = result["email"].toString();
  QSettings s;
  s.beginGroup(kSettingsGroup);
  s.setValue("user_email", user_email);
  ui_->login_state->SetLoggedIn(LoginStateWidget::LoggedIn, user_email);

  // Post a hello message to Glass
  QUrl url(kGlassTimelineUploadUrl);
  QNetworkRequest request(url);
  request.setRawHeader("Authorization",
      QString("Bearer %1").arg(access_token_).toUtf8());
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QString text = QString("Hello %1 at %2")
      .arg(result["email"].toString())
      .arg(QDateTime::currentDateTime().toString());
  QVariantMap data;
  data["text"] = text;
  QJson::Serializer json;

  qLog(Debug) << "Sending:" << data;
  QNetworkReply* post = network_->post(request, json.serialize(data));
  NewClosure(post, SIGNAL(finished()),
      this, SLOT(TimelineInsertFinished(QNetworkReply*)), post);
}

void GoogleGlassSettingsPage::TimelineInsertFinished(QNetworkReply* reply) {
  qLog(Debug) << Q_FUNC_INFO;
  reply->deleteLater();
  qLog(Debug) << reply->readAll();
}

bool GoogleGlassSettingsPage::eventFilter(QObject* object, QEvent* event) {
  if (object == dialog() && event->type() == QEvent::Enter) {
    ui_->login_button->setEnabled(true);
    return false;
  }

  return SettingsPage::eventFilter(object, event);
}

void GoogleGlassSettingsPage::LogoutClicked() {
  ui_->login_state->SetLoggedIn(LoginStateWidget::LoggedOut);
}
