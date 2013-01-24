#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QtDeclarative>
#include "cryptothread.h"
#include "passworddialog.h"
#include "encryptdecryptdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	void dropEvent(QDropEvent *);
	void dragEnterEvent(QDragEnterEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dragLeaveEvent(QDragLeaveEvent* event);
	bool isDecrypt(QStringList, bool *);
	Q_INVOKABLE void cancelCrypto();
	QString getOutputPath(bool);

private slots:
	void cryptoThreadDone();
	void cryptoError(QString);
	void cryptoComplete(QString);
	void cryptoStatusUpdate(ProgressType, float);

private:
	Ui::MainWindow *ui;
	PasswordDialog *passwordDialog;
	EncryptDecryptDialog *encryptDecryptDialog;
	CryptoThread *cryptoThread;
	QString fileExtension;
	QObject *qmlRootObject;
	QObject *ciphertextImage;
	QObject *plaintextImage;
	QObject *archiveImage;
	QObject *progressText;
};

#endif // MAINWINDOW_H
