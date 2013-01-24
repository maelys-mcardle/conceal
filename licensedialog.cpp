#include "licensedialog.h"
#include "ui_licensedialog.h"

LicenseDialog::LicenseDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::LicenseDialog)
{
	ui->setupUi(this);

	QFile gpl3File(":/text/LICENSE");
	if (gpl3File.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->gpl3Text->setPlainText(gpl3File.readAll());
		gpl3File.close();
	}

	QFile gpl2File(":/text/LICENSE-MCRYPT");
	if (gpl2File.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->gpl2Text->setPlainText(gpl2File.readAll());
		gpl2File.close();
	}

	QFile lgpl3File(":/text/oxygen-icons/LICENSE");
	if (lgpl3File.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->lgpl3Text->setPlainText(lgpl3File.readAll());
		lgpl3File.close();
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
