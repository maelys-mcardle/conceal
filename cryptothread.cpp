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
	MCRYPT td = mcrypt_module_open(MCRYPT_RIJNDAEL_128, NULL,
		MCRYPT_CBC, NULL);
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
	// Go through all the files given.
	foreach(QString encryptedFilePath, this->pathIn) {

		// Skip directories.
		if (QFileInfo(encryptedFilePath).isDir()) continue;

		// Decrypt files, then unload the archives.
		QTemporaryFile temporaryFile;
		this->decryptFile(td, encryptedFilePath, &temporaryFile);
		this->unarchiveFiles(encryptedFilePath,
			&temporaryFile, this->pathOut);
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
		this->renameTempFile(&encryptedFile, this->pathOut);
	}
}

void CryptoThread::renameTempFile(QTemporaryFile *file, QString path)
{
	// There might be a file in the way. Remove if so.
	if (!file->copy(path)) {
		QDir().remove(path);
		if (!file->copy(path)) {
			reportError("The computer wouldn't let me create the file\n"
				+ path);
		}
	}
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
			+ archiveFilePath);
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

	// Go through each entry.
	foreach (QString manifestEntry, manifestEntries) {

		// Load the entry.
		QStringList entryFields = manifestEntry.split(" ");
		if (entryFields.size() == 0) continue;

		// If it's a directory, create it.
		if (entryFields.at(0) == "d" && entryFields.size() > 0) {
			QDir().mkdir(entryFields.at(1));
			continue;
		}

		// If it's a file, create it.
		if (entryFields.at(0) == "f" && entryFields.size() > 1) {

			// Get the size and name of the file.
			quint64 fileSize = entryFields.at(1).toUInt();
			QString filePath = entryFields.at(2);

			// Create a temporary file.
			QTemporaryFile temporaryFile;
			if (!temporaryFile.open()) {
				reportError("The computer wouldn't let me create a\n"
					"temporary file, which is needed for this to work.");
				break;
			}

			// Store the contents.
			QByteArray block(this->copyChunkSize, 0);
			for (quint64 i = fileSize; i <= this->copyChunkSize;
				i -= this->copyChunkSize) {

				// See how much data to transfer.
				int amountToCopy = (i < this->copyChunkSize) ? i :
					this->copyChunkSize;

				// Transfer the data.
				in.readRawData(block.data(), amountToCopy);
				temporaryFile.write(block.data(), amountToCopy);
			}

			// Close the temporary file.
			temporaryFile.close();

			// Move the temporary file.
			this->renameTempFile(&temporaryFile, filePath);
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
	// Create the file manifest.
	QByteArray manifest = getFileManifest(inputFilePaths, pathRoot);

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

	// Store the challenge and the manifest.
	out << (quint32) valueA << (quint32) valueB << (quint32) valueXor
		<< manifest;

	// Store all the files.
	foreach (QString filePath, inputFilePaths) {

		// If it's a directory, go to the next item.
		if (QFileInfo(filePath).isDir()) continue;

		// If it's a file, open it.
		QFile plaintext(filePath);
		if (!plaintext.open(QIODevice::ReadOnly)) {
			reportError("The computer couldn't read the file\n"
				+ filePath);
			archiveFile->close();
			return false;
		}

		// Copy the data.
		while (!plaintext.atEnd()) {
			QByteArray plainData = plaintext.read(this->copyChunkSize);
			out << plainData;
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
	while (!plaintext->atEnd()) {

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

	// Success.
	return true;
}

bool CryptoThread::decryptFile(MCRYPT td, QString encryptedFilePath,
	QTemporaryFile *outputFile)
{
	// Open the encrypted file.
	QFile encryptedFile(encryptedFilePath);
	if (!encryptedFile.open(QIODevice::ReadOnly)) {
		reportError("The computer wouldn't let me open\n" +
			encryptedFilePath);
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
	quint64 firstBlock = true;
	while (!encryptedFile.atEnd()) {

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
		if (condensedPath.startsWith(QDir::separator()))
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
			QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot,
			QDirIterator::Subdirectories);

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

