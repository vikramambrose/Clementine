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

#include "exportcoversdialog.h"
#include "ui_exportcoversdialog.h"

#include <QSettings>

const char* ExportCoversDialog::kSettingsGroup = "ExportCoversDialog";

ExportCoversDialog::ExportCoversDialog(QWidget* parent)
  : QDialog(parent),
    ui_(new Ui_ExportCoversDialog)
{
  ui_->setupUi(this);
}

ExportCoversDialog::~ExportCoversDialog() {
  delete ui_;
}

ExportCoversDialog::DialogResult ExportCoversDialog::Exec() {
  // remembers last accepted settings
  QSettings s;
  s.beginGroup(kSettingsGroup);

  ui_->fileName->setText(s.value("fileName", "cover").toString());
  ui_->overwrite->setChecked(s.value("overwrite", false).toBool());

  DialogResult result = DialogResult();

  if(exec() != QDialog::Rejected) {
    QString fileName = ui_->fileName->text();
    bool overwrite = ui_->overwrite->isChecked();

    s.setValue("fileName", fileName);
    s.setValue("overwrite", overwrite);

    result.fileName_ = fileName;
    result.overwrite_ = overwrite;
  }

  return result;
}
