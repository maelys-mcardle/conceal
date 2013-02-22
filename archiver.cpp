#include "archiver.h"

Archiver::Archiver(QObject *parent) :
	QObject(parent)
{
	// When doing file-to-file copies, how much to displace at a time.
	this->copyChunkSize = 1048576;
}

ArchiverReturn Archiver::extractFile(QTemporaryFile *archiveFile,
	QString outputDir)
{
	// Jump to the directory where the files are to be dumped.
	QDir::setCurrent(outputDir);

	// Retrieve the manifest.
	QStringList fileList, dirList;
	QList <qint64> fileSizes;
	QDataStream archiveStream(archiveFile);
	archiveStream >> dirList >> fileList >> fileSizes;

	// Create the directories.
	foreach (QString directory, dirList) {
		if (!QDir().mkpath(directory)) return AR_FILE_UNWRITEABLE;
	}

	// Create the files.
	for (int i = 0; i < fileList.size(); i++) {

		// Open the file.
		QFile outputFile(fileList.at(i));
		if (!outputFile.open(QIODevice::WriteOnly)) {
			reportError("The program couldn't load in the file\n"
				"called \"" + fileList.at(i) + "\".");
			return AR_FILE_UNWRITEABLE;
		}

		// Copy the data.
		for (qint64 leftToRead = fileSizes.at(i); leftToRead > 0;
			leftToRead -= this->copyChunkSize) {

			// Determine how much to read.
			qint64 readAmount = (leftToRead > this->copyChunkSize) ?
				this->copyChunkSize : leftToRead;

			// Copy the data.
			QByteArray data = archiveFile->read(readAmount);
			outputFile.write(data.data(), data.size());
			updateProgress(PLAINTEXT_TO_ARCHIVE, (float)
				archiveFile->pos() / archiveFile->size());
		}

		// Close the file.
		outputFile.close();
	}

	return AR_OK;
}

ArchiverReturn Archiver::archiveFiles(QStringList inputFilePaths,
	QString pathRoot, QTemporaryFile *archiveFile)
{
	// Jump to the directory where the files are to be taken from.
	QDir::setCurrent(pathRoot);

	// Generate a manifest that gives the files, directories, and sizes.
	QStringList fileList, dirList;
	QList <qint64> fileSizes;
	foreach (QString inputPath, inputFilePaths) {
		if (QFileInfo(inputPath).isDir()) {
			dirList << inputPath.mid(pathRoot.size());
		} else {
			fileList << inputPath.mid(pathRoot.size());
			fileSizes << QFileInfo(inputPath).size();
		}
	}

	// Store the manifest.
	QDataStream archiveStream(archiveFile);
	archiveStream << dirList << fileList << fileSizes;

	// Write the files.
	for (int i = 0; i < fileList.size(); i++) {

		// Open the input file.
		QFile inputFile(fileList.at(i));
		if (!inputFile.open(QIODevice::ReadOnly)) {
			reportError("The program couldn't create the file\n"
				"called \"" + fileList.at(i) + "\".");
			return AR_FILE_UNREADABLE;
		}

		// Copy the data.
		while (!inputFile.atEnd()) {
			QByteArray data = inputFile.read(this->copyChunkSize);
			archiveFile->write(data.data(), data.size());
			updateProgressMany(PLAINTEXT_TO_ARCHIVE, inputFile.pos(),
					inputFile.size(), i, fileList.size());
		}

		// Close the input file.
		inputFile.close();
	}

	return AR_OK;
}

void Archiver::updateProgressMany(ProgressType type,
	qint64 positionInFile, qint64 fileSize, int fileNo, int totalFiles)
{
	float positionProgress = (float) positionInFile / (float) fileSize;
	float currentFileProgress = (float) fileNo / (float) totalFiles;
	float singleFileProgress = 1. / (float) totalFiles;
	updateProgress(type, currentFileProgress +
		positionProgress * singleFileProgress);
}
