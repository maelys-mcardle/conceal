#ifndef CRYPTOTHREAD_H
#define CRYPTOTHREAD_H

#include <QtGui>
extern "C" {
	#include <mcrypt.h>
}

typedef enum {
	PLAINTEXT_TO_ARCHIVE,
	ARCHIVE_TO_CIPHERTEXT,
	CIPHERTEXT_TO_ARCHIVE,
	ARCHIVE_TO_PLAINTEXT
} ProgressType;



class CryptoThread : public QThread
{
	Q_OBJECT
public:
	explicit CryptoThread(QObject *parent = 0);
	void setupRun(bool, QStringList, QString, QByteArray);
	void run();

signals:
	void updateProgress(ProgressType, float);
	void reportError(QString);
	void reportComplete(QString);

public slots:
	
private:
	void decryptFiles(MCRYPT);
	void encryptFiles(MCRYPT);
	bool archiveFiles(QStringList, QString, QTemporaryFile *);
	bool unarchiveFiles(QString, QTemporaryFile *, QString);
	bool encryptFile(MCRYPT, QTemporaryFile *, QTemporaryFile *);
	bool decryptFile(MCRYPT, QString, QTemporaryFile *);
	void renameTempFile(QTemporaryFile *, QString);
	QStringList getAllSubdirectories(QStringList);
	QString getRootPath(QStringList);
	QByteArray getFileManifest(QStringList, QString);
	bool decrypt;
	int encryptionBlockSize;
	quint64 copyChunkSize;
	QStringList pathIn;
	QString rootPathIn;
	QString pathOut;
	QByteArray key;
};

#endif // CRYPTOTHREAD_H
