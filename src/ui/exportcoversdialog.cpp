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

  connect(ui_->forceSize, SIGNAL(stateChanged(int)), SLOT(ForceSizeToggled(int)));
}

ExportCoversDialog::~ExportCoversDialog() {
  delete ui_;
}

ExportCoversDialog::DialogResult ExportCoversDialog::Exec() {
  QSettings s;
  s.beginGroup(kSettingsGroup);

  // restore last accepted settings
  ui_->fileName->setText(s.value("fileName", "cover").toString());
  ui_->overwrite->setChecked(s.value("overwrite", false).toBool());
  ui_->forceSize->setChecked(s.value("forceSize", false).toBool());
  ui_->width->setText(s.value("width", "").toString());
  ui_->height->setText(s.value("height", "").toString());

  ForceSizeToggled(ui_->forceSize->checkState());

  DialogResult result = DialogResult();

  if(exec() != QDialog::Rejected) {
    QString fileName = ui_->fileName->text();
    bool overwrite = ui_->overwrite->isChecked();
    bool forceSize = ui_->forceSize->isChecked();
    QString width = ui_->width->text();
    QString height = ui_->height->text();

    s.setValue("fileName", fileName);
    s.setValue("overwrite", overwrite);
    s.setValue("forceSize", forceSize);
    s.setValue("width", width);
    s.setValue("height", height);

    result.fileName_ = fileName;
    result.overwrite_ = overwrite;
  }

  return result;
}

void ExportCoversDialog::ForceSizeToggled(int state) {
  ui_->width->setEnabled(state == Qt::Checked);
  ui_->height->setEnabled(state == Qt::Checked);
}
