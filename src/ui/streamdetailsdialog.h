#ifndef STREAMDETAILSDIALOG_H
#define STREAMDETAILSDIALOG_H

#include <QDialog>

namespace Ui {
class StreamDetailsDialog;
}

class StreamDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StreamDetailsDialog(QWidget *parent = 0);
    ~StreamDetailsDialog();

private:
    Ui::StreamDetailsDialog *ui;
};

#endif // STREAMDETAILSDIALOG_H
