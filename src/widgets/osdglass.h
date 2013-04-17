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

#ifndef OSDGLASS_H
#define OSDGLASS_H

#include <QObject>

class NetworkAccessManager;
class OAuthenticator;
class QImage;
class QNetworkReply;

class OSDGlass : public QObject {
  Q_OBJECT
 public:
  OSDGlass(QObject* parent = 0);

  void ShowMessage(
      const QString& summary,
      const QString& message,
      const QString& icon,
      const QImage& image);

 private slots:
  void AuthoriseFinished(OAuthenticator* authenticator);
  void TimelineInsertFinished(QNetworkReply* reply);

 private:
  NetworkAccessManager* network_;
  QString access_token_;
};

#endif  // OSDGLASS_H
