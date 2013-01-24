#include "licensedialog.h"
#include "ui_licensedialog.h"

LicenseDialog::LicenseDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::LicenseDialog)
{
	ui->setupUi(this);

	QFile gpl3File(":/text/licenses/GPLv3");
	if (gpl3File.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->gpl3Text->setPlainText(gpl3File.readAll());
		gpl3File.close();
	}

	QFile gpl2File(":/text/licenses/GPLv2");
	if (gpl2File.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->gpl2Text->setPlainText(gpl2File.readAll());
		gpl2File.close();
	}

	QFile lgpl3File(":/text/licenses/LGPLv3");
	if (lgpl3File.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->lgpl3Text->setPlainText(lgpl3File.readAll());
		lgpl3File.close();
	}

	QFile lgpl21File(":/text/licenses/LGPLv21");
	if (lgpl21File.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->lgpl21Text->setPlainText(lgpl21File.readAll());
		lgpl21File.close();
	}
}

LicenseDialog::~LicenseDialog()
{
	delete ui;
}

void LicenseDialog::on_close_clicked()
{
	this->hide();
}
