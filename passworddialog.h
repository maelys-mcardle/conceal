#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QtGui>

namespace Ui {
class PasswordDialog;
}

class PasswordDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit PasswordDialog(QWidget *parent = 0);
	~PasswordDialog();
	QByteArray getPassword();
	void resetFields();

private slots:
	void on_cancel_clicked();
	void on_ok_clicked();

private:
	Ui::PasswordDialog *ui;
	QByteArray password;
};

#endif // PASSWORDDIALOG_H
