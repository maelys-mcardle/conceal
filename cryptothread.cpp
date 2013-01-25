#include "cryptothread.h"

CryptoThread::CryptoThread(QObject *parent) :
	QThread(parent)
{
	// When doing file-to-file copies, how much to displace at a time.
	this->copyChunkSize = 1048576;
}

void CryptoThread::run()
{
	// Missing parameters, abort.
	if (this->pathIn.size() == 0) return;

	// Open MCRYPT. Abort on fail.
	MCRYPT td = mcrypt_module_open(QByteArray(MCRYPT_RIJNDAEL_128).data(),
		NULL, QByteArray(MCRYPT_CBC).data(), NULL);
	if (td == MCRYPT_FAILED) {
		reportError("A part of the software that's needed to make\n"
			"the encryption happen broke. (Module open failure.)");
		return;
	}

	// Seed the pseudo-random number generator.
	QTime midnight(0, 0, 0);
	qsrand(midnight.secsTo(QTime::currentTime()));

	// Create the initialization vector.
	QByteArray IV(mcrypt_enc_get_iv_size(td), 0);
	for (int i = 0; i < IV.size(); i++)
		IV.replace(i, qrand());

	// Get the size of each block.
	this->encryptionBlockSize = mcrypt_enc_get_block_size(td);

	// Initialize. Return on fail.
	if (mcrypt_generic_init(td, this->key.data(), this->key.size(),
		IV.data()) < 0) {
		reportError("A part of the software that's needed to make\n"
			"the encryption happen broke. (Initialization failure.)");
		return;
	}

	// Apply encryption/decryption.
	if (this->decrypt) this->decryptFiles(td);
	else this->encryptFiles(td);

	// Finish up.
	mcrypt_generic_end(td);
}

void CryptoThread::decryptFiles(MCRYPT td)
{
	// Success count.
	int successes = 0, failures = 0;

	// Go through all the files given.
	foreach(QString encryptedFilePath, this->pathIn) {

		// Skip directories.
		if (QFileInfo(encryptedFilePath).isDir()) continue;

		// Decrypt files, then unload the archives.
		QTemporaryFile temporaryFile;
		if (this->decryptFile(td, encryptedFilePath, &temporaryFile) &&
			this->unarchiveFiles(encryptedFilePath, &temporaryFile,
			this->pathOut)) successes++;
		else failures++;
	}

	// Inform of situation.
	if (failures == 0) {
		reportComplete("All done. Your files are found here:\n" +
			toNativeSeparators(this->pathOut));
	} else if (successes > 0 && failures > 0) {
		reportComplete("Done, but there were problems reading\n"
			"some of the files.");
	}
}

void CryptoThread::encryptFiles(MCRYPT td)
{
	// Archive the files, then encrypt the archive.
	QTemporaryFile archiveFile, encryptedFile;
	bool archiveOK = this->archiveFiles(this->pathIn,
		this->rootPathIn, &archiveFile);
	bool encryptionOK = this->encryptFile(td, &archiveFile,
		&encryptedFile);

	// If all works, store the file.
	if (archiveOK && encryptionOK) {
		updateProgress(ENCRYPTION_MOVING_FILE, 1);
		if (this->renameTempFile(&encryptedFile, this->pathOut)) {
			reportComplete("All done. Your secure file is found here:\n"
				+ toNativeSeparators(this->pathOut));
		}
	}
}

bool CryptoThread::renameTempFile(QTemporaryFile *file, QString path)
{
	// There might be a file in the way. Remove if so.
	if (QFileInfo(path).exists()) {
		QDir().remove(path);
	}

	// Rename the file. If there was a problem, inform user.
	if (file->rename(path)) {
		file->setAutoRemove(false);
	} else {
		reportError("The computer wouldn't let me create the file\n"
			+ toNativeSeparators(path));
		return false;
	}

	return true;
}

bool CryptoThread::unarchiveFiles(QString archiveFilePath,
	QTemporaryFile *archiveFile, QString outputDir)
{
	// Open the output file.copyChunkSize
	if (!archiveFile->open()) {
		reportError("The computer wouldn't let me create a\n"
			"temporary file, which is needed for this to work.");
		return false;
	}

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
}

bool CryptoThread::archiveFiles(QStringList inputFilePaths,
	QString pathRoot, QTemporaryFile *archiveFile)
{
	// Open the file we'll store the contents to.
	if (!archiveFile->open()) {
		reportError("The computer wouldn't let me create a\n"
			"temporary file, which is needed for this to work.");
		return false;
	}
	QDataStream out(archiveFile);

	// Create a simple challenge to confirm the key on decryption.
	quint32 valueA = qrand(), valueB = qrand();
	quint32 valueXor = (valueA ^ valueB);
	out << (quint32) valueA << (quint32) valueB << (quint32) valueXor;

	// Store the manifest.
	out	<< getFileManifest(inputFilePaths, pathRoot);

	// Store all the files.
	for (int i = 0; i < inputFilePaths.size(); i++) {

		// If it's a directory, go to the next item.
		if (QFileInfo(inputFilePaths.at(i)).isDir()) continue;

		// If it's a file, open it.
		QFile plaintext(inputFilePaths.at(i));
		if (!plaintext.open(QIODevice::ReadOnly)) {
			reportError("The computer couldn't read the file\n"
				+ toNativeSeparators(inputFilePaths.at(i)));
			archiveFile->close();
			return false;
		}

		// Copy the data.
		float singleElementSize = 1. / inputFilePaths.size();
		while (!plaintext.atEnd()) {

			// Update the status for the UI.
			updateProgress(PLAINTEXT_TO_ARCHIVE,
				singleElementSize * i +
				singleElementSize * (float) plaintext.pos()
				/ (float) plaintext.size());

			// Copy the data.
			QByteArray buffer(this->copyChunkSize, 0);
			int bytesRead = plaintext.read(buffer.data(), buffer.size());
			archiveFile->write(buffer.data(), bytesRead);
		}

		// Close the unencrypted file.
		plaintext.close();
	}

	// Close out the encrypted file.
	archiveFile->close();

	// Success.
	return true;
}

bool CryptoThread::encryptFile(MCRYPT td, QTemporaryFile *plaintext,
	QTemporaryFile *ciphertext)
{
	// Open the files.
	if (!plaintext->open() || !ciphertext->open()) {
		reportError("The computer wouldn't let me create a\n"
			"temporary file, which is needed for this to work.");
		return false;
	}

	// The IV will not be communicated, so prepend a sacrificial block.
	QByteArray firstBlock(this->encryptionBlockSize, qrand());
	mcrypt_generic (td, firstBlock.data(), firstBlock.size());
	ciphertext->write(firstBlock.data(), firstBlock.size());

	// Encrypt the file.
	qint64 lastProgressUpdate = 0;
	while (!plaintext->atEnd()) {

		// Update the status for the UI.
		if (plaintext->pos() > lastProgressUpdate + this->copyChunkSize) {
			updateProgress(ARCHIVE_TO_CIPHERTEXT,
				(float) plaintext->pos() / (float) plaintext->size());
			lastProgressUpdate = plaintext->pos();
		}

		// Read the data in blocks.
		QByteArray block(this->encryptionBlockSize, 0);
		int dataRead = plaintext->read(block.data(), block.size());

		// Encrypt the data and write it.
		mcrypt_generic(td, block.data(), dataRead);
		ciphertext->write(block.data(), dataRead);
	}

	// Close the files.
	plaintext->close();
	ciphertext->close();

	return true;
}

bool CryptoThread::decryptFile(MCRYPT td, QString encryptedFilePath,
	QTemporaryFile *outputFile)
{
	// Open the encrypted file.
	QFile encryptedFile(encryptedFilePath);
	if (!encryptedFile.open(QIODevice::ReadOnly)) {
		reportError("The computer wouldn't let me open\n" +
			toNativeSeparators(encryptedFilePath));
		return false;
	}
	QDataStream stream(&encryptedFile);

	// Open the output file.
	if (!outputFile->open()) {
		reportError("The computer wouldn't let me create a\n"
			"temporary file, which is needed for this to work.");
		return false;
	}

	// Decrypt the file.
	qint64 firstBlock = true;
	qint64 lastProgressUpdate = 0;
	while (!encryptedFile.atEnd()) {	

		// Update the status for the UI.
		if (encryptedFile.pos() > lastProgressUpdate + this->copyChunkSize) {
			updateProgress(CIPHERTEXT_TO_ARCHIVE,
				(float) encryptedFile.pos() / (float) encryptedFile.size());
			lastProgressUpdate = encryptedFile.pos();
		}

		// Read the data in blocks.
		QByteArray block(this->encryptionBlockSize, 0);
		int dataRead = stream.readRawData(block.data(), block.size());

		// Decrypt the data.
		mdecrypt_generic (td, block.data(), dataRead);

		// Skip the first block, which was mangled by the random IV.
		if (firstBlock) { firstBlock = false; continue; }

		// Write the data back.
		outputFile->write(block.data(), dataRead);
	}

	// Close the encrypted file.
	encryptedFile.close();
	outputFile->close();

	// Report success.
	return true;
}

QByteArray CryptoThread::getFileManifest(QStringList filePaths,
	QString fileBase)
{
	// Create the blank manifest.
	QByteArray manifest;

	// Go file by file. Load it's type, size, load it's location.
	foreach (QString filePath, filePaths) {

		QFileInfo info(filePath);
		QString condensedPath = filePath.remove(0, fileBase.size());
		if (condensedPath.startsWith("/"))
			condensedPath.remove(0, 1);

		if (info.isDir()) {
			manifest.append("d " + condensedPath.toUtf8() + "\n");
		} else {
			manifest.append(QString("f %1 ").arg(info.size()).toUtf8());
			manifest.append(condensedPath.toUtf8() + "\n");
		}
	}

	return manifest;
}

QString CryptoThread::toNativeSeparators(QString path)
{
	QString pathCopy = path;
	return pathCopy.replace("/", QDir::separator());
}

void CryptoThread::setupRun(bool isDecrypt, QStringList inputFiles,
	QString outputPath, QByteArray encryptionKey)
{
	// Missing critical parameters, abort.
	if (inputFiles.size() == 0) return;

	this->decrypt = isDecrypt;
	this->pathIn = getAllSubdirectories(inputFiles);
	this->rootPathIn = getRootPath(inputFiles);
	this->pathOut = outputPath;
	this->key = encryptionKey;
}

QStringList CryptoThread::getAllSubdirectories(QStringList inputPaths)
{
	// Setup the output.
	QStringList outputPaths;

	// Find all the subdirectories and files contained by the
	// file paths specified.
	foreach (QString inputPath, inputPaths) {

		outputPaths << inputPath;

		QDirIterator directories(inputPath,
			QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot |
			QDir::Hidden, QDirIterator::Subdirectories);

		while(directories.hasNext()){
			directories.next();
			outputPaths << directories.filePath();
		}
	}

	// Remove copies of the same paths.
	outputPaths.removeDuplicates();
	return outputPaths;
}


QString CryptoThread::getRootPath(QStringList inputPaths)
{
	// Setup the output.
	QDir rootDirectory = QFileInfo(inputPaths.at(0)).absoluteDir();

	// Find the common directory that belies all entries.
	do {
		bool mismatch = false;
		for (int i = 1; i < inputPaths.size(); i++) {
			if (!inputPaths.at(i).startsWith(
				rootDirectory.absolutePath())) {
				mismatch = true;
				break;
			}
		}
		if (mismatch == false) break;
	} while (rootDirectory.cdUp());

	// Split the first path by directories.
	return rootDirectory.absolutePath();
}

