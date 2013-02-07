#ifndef ARCHIVER_H
#define ARCHIVER_H

#include <QtGui>
#include "progresstype.h"
extern "C" {
	#include <archive.h>
	#include <archive_entry.h>
	#include <fcntl.h>
}

typedef enum {
	AR_OK,
	AR_ARCHIVE_ERROR,
	AR_FILE_UNREADABLE
} ArchiverReturn;

class Archiver : public QObject
{
	Q_OBJECT
public:
	explicit Archiver(QObject *parent = 0);
	ArchiverReturn compressFiles(QStringList, QString, QTemporaryFile *);
	ArchiverReturn extractFiles(QString, QTemporaryFile *, QString);

signals:
	void updateProgress(ProgressType, float);
	void reportError(QString);
	void reportComplete(QString);

public slots:
	
private:
	int copyArchiveData(struct archive *, struct archive *);
	void updateProgressMany(ProgressType, qint64, qint64, int, int);
	qint64 copyChunkSize;
};

#endif // ARCHIVER_H
