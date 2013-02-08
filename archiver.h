#ifndef ARCHIVER_H
#define ARCHIVER_H

#include <QtGui>
#include "progresstype.h"

typedef enum {
	AR_OK,
	AR_ARCHIVE_ERROR,
	AR_FILE_UNREADABLE,
	AR_FILE_UNWRITEABLE
} ArchiverReturn;

class Archiver : public QObject
{
	Q_OBJECT
public:
	explicit Archiver(QObject *parent = 0);
	ArchiverReturn archiveFiles(QStringList, QString, QTemporaryFile *);
	ArchiverReturn extractFile(QTemporaryFile *, QString);

signals:
	void updateProgress(ProgressType, float);
	void reportError(QString);

public slots:
	
private:
	ArchiverReturn runExtraction(QTemporaryFile *);
	ArchiverReturn runArchive(QStringList, QString, QTemporaryFile *);
	void updateProgressMany(ProgressType, qint64, qint64, int, int);
	qint64 copyChunkSize;
};

#endif // ARCHIVER_H
