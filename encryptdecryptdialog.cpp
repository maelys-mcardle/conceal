#include "encryptdecryptdialog.h"
#include "ui_encryptdecryptdialog.h"

EncryptDecryptDialog::EncryptDecryptDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::EncryptDecryptDialog)
{
	ui->setupUi(this);
}

EncryptDecryptDialog::~EncryptDecryptDialog()
{
	delete ui;
}

void EncryptDecryptDialog::on_encrypt_clicked()
{
	this->actionEncrypt = true;
	this->done(QDialog::Accepted);
}

void EncryptDecryptDialog::on_decrypt_clicked()
{
	this->actionEncrypt = false;
	this->done(QDialog::Accepted);
}

bool EncryptDecryptDialog::isEncrypt()
{
	return this->actionEncrypt;
}
