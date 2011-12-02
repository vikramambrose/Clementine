/* This file is part of Clementine.
   Copyright 2011, David Sansome <me@davidsansome.com>

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

#ifndef SPEECHANALYZERWIDGET_H
#define SPEECHANALYZERWIDGET_H

#include <QWidget>

class SpeechAnalyzerWidget : public QWidget {
  Q_OBJECT

public:
  SpeechAnalyzerWidget(QWidget* parent = 0);

public slots:
  void AudioLevelChanged(float audio_level);

};

#endif // SPEECHANALYZERWIDGET_H
