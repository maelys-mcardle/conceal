#include "passworddialog.h"
#include "ui_passworddialog.h"

PasswordDialog::PasswordDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PasswordDialog)
{
	ui->setupUi(this);
}

PasswordDialog::~PasswordDialog()
{
	delete ui;
}

void PasswordDialog::resetFields()
{
	ui->password->setFocus();
	ui->password->clear();
	ui->passwordConfirm->clear();
}

void PasswordDialog::on_cancel_clicked()
{
	this->resetFields();
	this->done(QDialog::Rejected);
}

void PasswordDialog::on_ok_clicked()
{
	// Passwords do not match.
	if (ui->password->text() != ui->passwordConfirm->text()) {
		QMessageBox::warning(this, "Password Mismatch",
			"The two passwords you entered were different.\n"
			"Please enter them again.",
			QMessageBox::Ok);

	// Passwords are blank.
	} else if (ui->password->text() == "") {
		QMessageBox::warning(this, "No Password Entered",
			"You did not specify a password.\n"
			"Please enter one to continue.",
			QMessageBox::Ok);

	// Password is OK. Store its hash.
	} else {
		QCryptographicHash hash(QCryptographicHash::Md5);
		for (int i = 0; i < 256; i++) hash.addData(QByteArray(1, i));
		hash.addData(ui->password->text().toLocal8Bit());
		this->hash = hash.result();
		this->resetFields();
		this->done(QDialog::Accepted);
	}
}

QByteArray PasswordDialog::getHash()
{
	return this->hash;
}
