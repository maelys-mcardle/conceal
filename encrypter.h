#ifndef ENCRYPTER_H
#define ENCRYPTER_H

#include <QtGui>
#include "progresstype.h"
extern "C" {
	#include <mcrypt.h>
}

typedef enum {
	ER_OK,
	ER_PASSWORD_WRONG,
	ER_MCRYPT_OPEN_ERROR,
	ER_MCRYPT_INIT_ERROR
} EncrypterReturn;

class Encrypter : public QObject
{
	Q_OBJECT
public:
	explicit Encrypter(QObject *parent = 0);
	~Encrypter();
	EncrypterReturn setKey(QByteArray);
	EncrypterReturn encryptFile(QTemporaryFile *, QTemporaryFile *);
	EncrypterReturn decryptFile(QFile *, QTemporaryFile *);

signals:
	void updateProgress(ProgressType, float);
	void reportError(QString);
	
public slots:

private:
	MCRYPT td;
	QByteArray generateChallenge();
	QByteArray generateRandomBlock();
	bool validateChallenge(QByteArray);
	int progressUpdateInterval;
	int encryptionBlockSize;
};

#endif // ENCRYPTER_H
