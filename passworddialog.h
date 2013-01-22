#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QtGui>
#include <QCryptographicHash>

namespace Ui {
class PasswordDialog;
}

class PasswordDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit PasswordDialog(QWidget *parent = 0);
	~PasswordDialog();
	QByteArray getHash();
	void resetFields();

private slots:
	void on_cancel_clicked();
	void on_ok_clicked();

private:
	Ui::PasswordDialog *ui;
	QByteArray hash;

};

#endif // PASSWORDDIALOG_H
