#ifndef ENCRYPTDECRYPTDIALOG_H
#define ENCRYPTDECRYPTDIALOG_H

#include <QDialog>

namespace Ui {
class EncryptDecryptDialog;
}

class EncryptDecryptDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit EncryptDecryptDialog(QWidget *parent = 0);
	~EncryptDecryptDialog();
	bool isEncrypt();

private slots:
	void on_decrypt_clicked();
	void on_encrypt_clicked();

private:
	Ui::EncryptDecryptDialog *ui;
	bool actionEncrypt;
};

#endif // ENCRYPTDECRYPTDIALOG_H
