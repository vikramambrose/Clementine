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

#include "config.h"

#include "plugindialog.h"
#include "pluginmodel.h"
#include "ui_plugindialog.h"
#include "core/boundfuturewatcher.h"
#include "ui/iconloader.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QtDebug>

const int PluginDelegate::kIconSize = 64;
const int PluginDelegate::kPadding = 6;
const int PluginDelegate::kItemHeight = kIconSize + kPadding*2;
const int PluginDelegate::kLinkSpacing = 10;

const char* PluginDialog::kSettingsGroup = "PluginDialog";

PluginDelegate::PluginDelegate(QObject* parent)
  : QStyledItemDelegate(parent),
    bold_metrics_(bold_)
{
  bold_.setBold(true);
  bold_metrics_ = QFontMetrics(bold_);

  link_.setUnderline(true);
}

void PluginDelegate::paint(
    QPainter* p, const QStyleOptionViewItem& opt,
    const QModelIndex& index) const {
  // Draw the background
  const QStyleOptionViewItemV3* vopt = qstyleoption_cast<const QStyleOptionViewItemV3*>(&opt);
  const QWidget* widget = vopt->widget;
  QStyle* style = widget->style() ? widget->style() : QApplication::style();
  style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, p, widget);

  // Get the data
  QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
  const QString title = index.data(Qt::DisplayRole).toString();
  const QString description = index.data(PluginModel::Role_Description).toString();
  const bool is_enabled = index.data(PluginModel::Role_IsEnabled).toBool();

  // Calculate the geometry
  QRect icon_rect(opt.rect.left() + kPadding, opt.rect.top() + kPadding,
                  kIconSize, kIconSize);
  QRect title_rect(icon_rect.right() + kPadding, icon_rect.top(),
                   opt.rect.width() - icon_rect.width() - kPadding*3,
                   bold_metrics_.height());
  QRect description_rect(title_rect.left(), title_rect.bottom(),
                         title_rect.width(), opt.rect.bottom() - kPadding - title_rect.bottom());

  // Draw the icon
  p->drawPixmap(icon_rect, icon.pixmap(kIconSize, is_enabled ? QIcon::Normal : QIcon::Disabled));

  // Disabled items get greyed out
  if (is_enabled) {
    p->setPen(opt.palette.color(QPalette::Text));
  } else {
    const bool light = opt.palette.color(QPalette::Base).value() > 128;
    const QColor color = opt.palette.color(QPalette::Dark);
    p->setPen(light ? color.darker(150) : color.lighter(125));
  }

  // Draw the title
  p->setFont(bold_);
  p->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter, title);

  // Draw the description
  p->setFont(opt.font);
  p->drawText(description_rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, description);
}

QSize PluginDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
  return QSize(100, kItemHeight);
}


PluginDialog::PluginDialog(QWidget* parent)
  : QDialog(parent),
    ui_(new Ui_PluginDialog),
    model_(NULL)
{
  ui_->setupUi(this);
  connect(ui_->enable, SIGNAL(clicked()), SLOT(Enable()));
  connect(ui_->disable, SIGNAL(clicked()), SLOT(Disable()));
  connect(ui_->settings, SIGNAL(clicked()), SLOT(Settings()));
  connect(ui_->reload, SIGNAL(clicked()), SLOT(Reload()));

  // Add a button to install a script from a file
  QPushButton* install_button = new QPushButton(
        IconLoader::Load("document-open"), tr("Install from file..."), this);
  connect(install_button, SIGNAL(clicked()), SLOT(InstallFromFile()));
  ui_->button_box->addButton(install_button, QDialogButtonBox::ActionRole);

  // But disable it if we don't have libarchive
#ifndef HAVE_LIBARCHIVE
  install_button->setEnabled(false);
#endif

  // Start on the first tab
  ui_->tab_widget->setCurrentIndex(0);

  // Make the list pretty
  ui_->list->setItemDelegate(new PluginDelegate(this));

  // Hide developer mode stuff by default
  ui_->console->hide();
  ui_->reload->hide();

  // Make the dialog smaller
  resize(width(), minimumSizeHint().height());

  ui_->button_box->button(QDialogButtonBox::Close)->setShortcut(QKeySequence::Close);

  ReloadSettings();
}

PluginDialog::~PluginDialog() {
  delete ui_;
}

void PluginDialog::SetModel(PluginModel* model) {
  model_ = model;
  ui_->list->setModel(model);

  connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          SLOT(DataChanged(QModelIndex,QModelIndex)));
  connect(ui_->list->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
          SLOT(CurrentChanged(QModelIndex)));
}

void PluginDialog::CurrentChanged(const QModelIndex& index) {
  if (!index.isValid()) {
    ui_->enable->setEnabled(false);
    ui_->disable->setEnabled(false);
    ui_->settings->setEnabled(false);
    ui_->reload->setEnabled(false);
    return;
  }

  const bool is_enabled = index.data(PluginModel::Role_IsEnabled).toBool();
  ui_->enable->setEnabled(!is_enabled);
  ui_->disable->setEnabled(is_enabled);
  ui_->settings->setEnabled(is_enabled);
  ui_->reload->setEnabled(is_enabled);
}

void PluginDialog::DataChanged(const QModelIndex&, const QModelIndex&) {
  CurrentChanged(ui_->list->currentIndex());
}

void PluginDialog::Enable() {
  model_->Enable(ui_->list->currentIndex());
}

void PluginDialog::Disable() {
  model_->Disable(ui_->list->currentIndex());
}

void PluginDialog::Settings() {
  //model_->ShowSettingsDialog(ui_->list->currentIndex());
}

void PluginDialog::Reload() {
  Disable();
  Enable();
}

void PluginDialog::ReloadSettings() {
  QSettings s;
  s.beginGroup(kSettingsGroup);
  last_open_dir_ = s.value("last_open_dir", QDir::homePath()).toString();
}

void PluginDialog::InstallFromFile() {
  /*QString filename = QFileDialog::getOpenFileName(
        this, tr("Install script file"), last_open_dir_,
        tr("Clementine scripts") + " (*.tar.gz *.clem)");
  if (filename.isEmpty())
    return;

  // Save this directory
  last_open_dir_ = filename;
  QSettings s;
  s.beginGroup(kSettingsGroup);
  s.setValue("last_open_dir", last_open_dir_);

  // Try opening the archive.  We do this in the background.
  ScriptArchive* archive = new ScriptArchive(model_);
  QFuture<bool> future = archive->LoadFromFileAsync(filename);

  BoundFutureWatcher<bool, ScriptArchive*>* watcher =
      new BoundFutureWatcher<bool, ScriptArchive*>(archive);
  watcher->setFuture(future);

  connect(watcher, SIGNAL(finished()), SLOT(InstallFromFileLoaded()));*/
}

void PluginDialog::InstallFromFileLoaded() {
  /*BoundFutureWatcher<bool, ScriptArchive*>* watcher =
      reinterpret_cast<BoundFutureWatcher<bool, ScriptArchive*>*>(sender());
  watcher->deleteLater();

  ScriptArchive* archive = watcher->data();
  const bool success = watcher->result();

  if (success) {
    // The dialog will delete itself and the archive when it's done.
    InstallPluginDialog* dialog = new InstallPluginDialog(archive, this);
    dialog->show();
  } else {
    QMessageBox::warning(this, tr("Error opening script archive"),
      tr("This is not a valid Clementine script file."), QMessageBox::Close);
    delete archive;
  }*/
}
