/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>

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

#ifndef PLUGINDIALOG_H
#define PLUGINDIALOG_H

#include <QDialog>
#include <QFont>
#include <QFontMetrics>
#include <QStyledItemDelegate>

class PluginModel;
class Ui_PluginDialog;

class PluginDelegate : public QStyledItemDelegate {
public:
  PluginDelegate(QObject* parent = 0);

  static const int kIconSize;
  static const int kPadding;
  static const int kItemHeight;
  static const int kLinkSpacing;

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const;

private:
  QFont bold_;
  QFontMetrics bold_metrics_;

  QFont link_;
};

class PluginDialog : public QDialog {
  Q_OBJECT

public:
  PluginDialog(QWidget* parent = 0);
  ~PluginDialog();

  static const char* kSettingsGroup;

  void SetModel(PluginModel* model);

private slots:
  void DataChanged(const QModelIndex& top_left, const QModelIndex& bottom_right);
  void CurrentChanged(const QModelIndex& index);
  void Enable();
  void Disable();
  void Settings();
  void Reload();

  void InstallFromFile();
  void InstallFromFileLoaded();

private:
  void ReloadSettings();

private:
  Ui_PluginDialog* ui_;

  PluginModel* model_;

  QString last_open_dir_;
};

#endif // PLUGINDIALOG_H
