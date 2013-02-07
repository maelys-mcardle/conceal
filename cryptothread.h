#ifndef CRYPTOTHREAD_H
#define CRYPTOTHREAD_H

#include <QtGui>
#include "archiver.h"
#include "encrypter.h"
#include "progresstype.h"

class CryptoThread : public QThread
{
	Q_OBJECT
public:
	explicit CryptoThread(QObject *parent = 0);
	~CryptoThread();
	void setupRun(bool, QStringList, QString, QByteArray);
	void run();

signals:
	void updateProgress(ProgressType, float);
	void reportError(QString);
	void reportComplete(QString);

public slots:
	
private:
	Encrypter *encrypter;
	Archiver *archiver;
	void decryptFiles();
	void encryptFiles();
	bool renameTempFile(QTemporaryFile *, QString);
	QString toNativeSeparators(QString);
	QStringList getAllSubdirectories(QStringList);
	QString getRootPath(QStringList);
	bool actionEncrypt;
	QStringList pathIn;
	QString rootPathIn;
	QString pathOut;
	QByteArray key;
};

#endif // CRYPTOTHREAD_H
