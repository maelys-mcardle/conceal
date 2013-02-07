#include "archiver.h"

Archiver::Archiver(QObject *parent) :
	QObject(parent)
{
	// When doing file-to-file copies, how much to displace at a time.
	this->copyChunkSize = 1048576;
}


ArchiverReturn Archiver::extractFiles(QString archiveFilePath,
	QTemporaryFile *archiveFile, QString outputDir)
{
	int r;

	int flags = ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;


	struct archive *a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_compression_all(a);
	struct archive *ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);
	struct archive_entry *entry;

	//if ((r = archive_read_open_fd(a, archiveFile->handle(), this->copyChunkSize)))
	//	return false;
	r = archive_read_open_fd(a, archiveFile->handle(), this->copyChunkSize);
	qDebug("%i %s", r, archiveFile->fileName().toLocal8Bit().data());
	//if ((r = archive_read_open_filename(a, "/home/julien/Development/decrypted", this->copyChunkSize)))
	//	return false;

	while (true) {

		r = archive_read_next_header(a, &entry);
		if (r == ARCHIVE_EOF) break;
		if (r != ARCHIVE_OK) qDebug("%s\n", archive_error_string(a));
		if (r < ARCHIVE_WARN) return AR_ARCHIVE_ERROR;

		r = archive_write_header(ext, entry);
		if (r != ARCHIVE_OK) qDebug("%s\n", archive_error_string(ext));
		else if (archive_entry_size(entry) > 0) {


			copyArchiveData(a, ext);

			if (r != ARCHIVE_OK)
			qDebug("%s\n", archive_error_string(ext));
			if (r < ARCHIVE_WARN) return AR_ARCHIVE_ERROR;
			updateProgress(ARCHIVE_TO_PLAINTEXT, 0);

		}

		r = archive_write_finish_entry(ext);
		if (r != ARCHIVE_OK) qDebug("%s\n", archive_error_string(ext));
		if (r < ARCHIVE_WARN) return AR_ARCHIVE_ERROR;
	}
	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);

	return AR_OK;
}

int Archiver::copyArchiveData(struct archive *ar, struct archive *aw)
{
  int r;
  const void *buff;
  size_t size;
  off_t offset;

  for (;;) {
	r = archive_read_data_block(ar, &buff, &size, &offset);
	if (r == ARCHIVE_EOF)
	  return (ARCHIVE_OK);
	if (r != ARCHIVE_OK)
	  return (r);
	r = archive_write_data_block(aw, buff, size, offset);
	if (r != ARCHIVE_OK) {
	  fprintf(stderr, "%s\n", archive_error_string(aw));
	  return (r);
	}
  }
}

/*
	// Open the encrypted file.
	QDataStream in(archiveFile);

	// Retrieve the challenge. If it fails, inform the user.
	quint32 valueA, valueB, valueXor;
	in >> valueA >> valueB >> valueXor;
	if ((valueXor ^ valueB) != valueA) {
		reportError("The password wasn't the right one for the file\n"
			+ toNativeSeparators(archiveFilePath));
		return false;
	}

	// Load the manifest.
	QByteArray manifest;
	in >> manifest;

	// Parse the manifest into UTF-8, and retrieve the entries.
	QStringList manifestEntries = QString().fromUtf8(manifest.data(),
		manifest.size()).split("\n");

	// Switch over to the output directory.
	QString initialDirectory = QDir::currentPath();
	QDir::setCurrent(outputDir);

	// Count the number of files.
	int numberOfFiles = 0;
	foreach (QString entry, manifestEntries) {
		if (entry.startsWith("f")) numberOfFiles++;
	}

	// Go through each entry.
	int fileCount = 0;
	for (int entryNo = 0; entryNo < manifestEntries.size(); entryNo++) {

		// Load the entry.
		QStringList entryFields = manifestEntries.at(entryNo).split(" ");
		if (entryFields.size() == 0) continue;

		// If it's a directory, create it.
		if (entryFields.at(0) == "d" && entryFields.size() > 0) {
			entryFields.removeAt(0);
			QDir().mkpath(entryFields.join(" "));
			continue;
		}

		// If it's a file, create it.
		if (entryFields.at(0) == "f" && entryFields.size() > 1) {

			// Get the size and name of the file.
			qint64 fileSize = entryFields.at(1).toInt();
			QString filePath = entryFields.at(2);
			for (int i = 3; i < entryFields.size(); i++)
				filePath += " " + entryFields.at(i);

			// Create a temporary file.
			QTemporaryFile temporaryFile;
			if (!temporaryFile.open()) {
				reportError("The computer wouldn't let me create a\n"
					"temporary file, which is needed for this to work.");
				break;
			}

			// Store the contents.
			QByteArray block(this->copyChunkSize, 0);
			float singleEntrySize = 1. / (float) numberOfFiles;
			for (qint64 i = fileSize ;; i -= this->copyChunkSize) {

				// Update the status for the UI.
				updateProgress(ARCHIVE_TO_PLAINTEXT,
					singleEntrySize * (float) fileCount +
					singleEntrySize * (float) archiveFile->pos() /
					(float) archiveFile->size());

				// See how much data to transfer.
				int amountToCopy = (i < this->copyChunkSize) ? i :
					this->copyChunkSize;

				// Transfer the data.
				in.readRawData(block.data(), amountToCopy);
				temporaryFile.write(block.data(), amountToCopy);

				// If this is the last block, stop.
				if (i <= this->copyChunkSize) break;
			}

			// Close the temporary file.
			temporaryFile.close();

			// Move the temporary file.
			updateProgress(DECRYPTION_MOVING_FILE, 1);
			if (!this->renameTempFile(&temporaryFile, filePath))
				return false;

			// Keep track of the files.
			fileCount++;
		}
	}

	// Restore the working directory.
	QDir::setCurrent(initialDirectory);

	// Close the archive file.
	archiveFile->close();

	// Success.
	return true;
}*/

ArchiverReturn Archiver::compressFiles(QStringList inputFilePaths,
	QString pathRoot, QTemporaryFile *archiveFile)
{
	// Set-up a new archive, which points to that file.
	struct archive *archive = archive_write_new();
	archive_write_add_filter_gzip(archive);
	archive_write_set_format_pax_restricted(archive);
	archive_write_open_fd(archive, archiveFile->handle());

	// Go file by file.
	for (int i = 0; i < inputFilePaths.size(); i++) {

		// Get the file path.
		QString filePath = inputFilePaths.at(i);

		// Set-up a new entry in the archive.
		struct archive_entry *entry = archive_entry_new();
		archive_entry_set_pathname(entry, filePath.toLocal8Bit().data());
		archive_entry_set_perm(entry, 0644);

		// Handle directories.
		if (QFileInfo(filePath).isDir()) {
			archive_entry_set_filetype(entry, AE_IFDIR);
			archive_entry_set_size(entry, 0);
			updateProgress(PLAINTEXT_TO_ARCHIVE, (float) i /
				inputFilePaths.size());
		}

		// Handle files.
		else {

			// Open up the file to archive.
			QFile rawFile(filePath);
			if (!rawFile.open(QIODevice::ReadOnly)) {
				reportError("The computer couldn't read the file\n"
					+ QFileInfo(filePath).fileName());
				archiveFile->close();
				return AR_FILE_UNREADABLE;
			}

			// Load in the details.
			archive_entry_set_filetype(entry, AE_IFREG);
			archive_entry_set_size(entry, rawFile.size());

			// Load in the data into the archive.
			while (!rawFile.atEnd()) {
				QByteArray buffer(this->copyChunkSize, 0);
				int bytesRead = rawFile.read(buffer.data(), buffer.size());
				archive_write_data(archive, buffer.data(), bytesRead);
				updateProgressMany(PLAINTEXT_TO_ARCHIVE, rawFile.pos(),
					rawFile.size(), i, inputFilePaths.size());
			}

			// Close the archive file.
			rawFile.close();
		}

		// Store the generated header.
		archive_write_header(archive, entry);
		archive_entry_free(entry);
	}

	// Close the archive.
	archiveFile->close();
	archive_write_close(archive);
	archive_write_free(archive);
	return AR_OK;
}

void Archiver::updateProgressMany(ProgressType type,
	qint64 positionInFile, qint64 fileSize, int fileNo, int totalFiles)
{
	float positionProgress = (float) positionInFile / fileSize;
	float currentFileProgress = fileNo / totalFiles;
	float singleFileProgress = 1 / totalFiles;
	updateProgress(type, currentFileProgress +
		positionProgress * singleFileProgress);
}
