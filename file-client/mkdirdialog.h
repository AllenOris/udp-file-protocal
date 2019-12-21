#ifndef MKDIRDIALOG_H
#define MKDIRDIALOG_H

#include <QDialog>
class dlg;

namespace Ui {
class MkdirDialog;
}

class MkdirDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MkdirDialog(dlg *, QWidget *parent = 0);
    ~MkdirDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::MkdirDialog *ui;
    dlg* m_dlg;
};

#endif // MKDIRDIALOG_H
