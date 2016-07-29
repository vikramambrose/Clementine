#include "streamdetailsdialog.h"
#include "ui_streamdetailsdialog.h"

StreamDetailsDialog::StreamDetailsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StreamDetailsDialog)
{
    ui->setupUi(this);
}

StreamDetailsDialog::~StreamDetailsDialog()
{
    delete ui;
}
