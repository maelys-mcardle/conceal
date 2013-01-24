#include "licensedialog.h"
#include "ui_licensedialog.h"

LicenseDialog::LicenseDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::LicenseDialog)
{
	ui->setupUi(this);

	QFile gplFile(":/text/LICENSE");
	if (gplFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->gplText->setPlainText(gplFile.readAll());
		gplFile.close();
	}

	QFile oxygenFile(":/text/oxygen-icons/README");
	if (oxygenFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->oxygenText->setPlainText(oxygenFile.readAll());
		oxygenFile.close();
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
