#include "mkdirdialog.h"
#include "ui_mkdirdialog.h"
#include "dlg.h"

MkdirDialog::MkdirDialog(dlg *m_dlg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MkdirDialog),
    m_dlg(m_dlg)
{
    ui->setupUi(this);
    ui->m_text->setFocus();
}

MkdirDialog::~MkdirDialog()
{
    delete ui;
}

void MkdirDialog::on_buttonBox_accepted()
{
    m_dlg->mkdirPath=ui->m_text->text();
}
