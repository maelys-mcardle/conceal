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

void EncryptDecryptDialog::on_decrypt_clicked()
{
	this->decrypt = true;
	this->done(QDialog::Accepted);
}

void EncryptDecryptDialog::on_encrypt_clicked()
{
	this->decrypt = false;
	this->done(QDialog::Accepted);
}

bool EncryptDecryptDialog::isDecrypt()
{
	return this->decrypt;
}
