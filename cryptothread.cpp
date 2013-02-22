#include "cryptothread.h"

CryptoThread::CryptoThread(QObject *parent) :
	QThread(parent)
{
	// Generate encrypter, archiver.
	this->encrypter = new Encrypter(this);
	this->archiver = new Archiver(this);

	// Connect signals/slots.
	connect(this->encrypter, SIGNAL(reportError(QString)),
		this, SLOT(spreadError(QString)));
	connect(this->encrypter, SIGNAL(updateProgress(ProgressType,float)),
		this, SLOT(spreadUpdate(ProgressType,float)));
	connect(this->archiver, SIGNAL(reportError(QString)),
		this, SLOT(spreadError(QString)));
	connect(this->archiver, SIGNAL(updateProgress(ProgressType,float)),
		this, SLOT(spreadUpdate(ProgressType,float)));
}

void CryptoThread::spreadError(QString message)
{
	reportError(message);
}

void CryptoThread::spreadUpdate(ProgressType type, float progress)
{
	updateProgress(type, progress);
}

void CryptoThread::run()
{
	// Missing parameters, abort.
	if (this->pathIn.size() == 0) return;

	// Set the key with the encrypter.
	EncrypterReturn encrypterStatus = this->encrypter->setKey(this->key);
	if (encrypterStatus != ER_OK) {
		reportError("A part of the software that's needed to make\n"
			"the encryption happen broke.");
		return;
	}

	// Apply encryption or decryption.
	if (this->actionEncrypt) this->encryptFiles();
	else this->decryptFiles();
}

void CryptoThread::decryptFiles()
{
	// Success count.
	int successes = 0, failures = 0;

	// Go through all the files given.
	foreach(QString encryptedFilePath, this->pathIn) {

		// Skip directories.
		if (QFileInfo(encryptedFilePath).isDir()) continue;

		// Decrypt the individual files.
		if (this->decryptFile(encryptedFilePath)) successes++;
		else failures++;
	}

	// Inform of situation.
	if (failures == 0)
		reportComplete("Decryption Complete",
			"All done. Your files are found here:\n" +
			toNativeSeparators(this->pathOut));
	else if (successes > 0 && failures > 0)
		reportComplete("Decryption Complete",
			"Done, but there were problems reading\n"
			"some of the files.");

}

bool CryptoThread::decryptFile(QString encryptedFilePath)
{
	// Open the encrypted file.
	QFile encryptedFile(encryptedFilePath);
	if (!encryptedFile.open(QIODevice::ReadOnly)) {
		reportError("The computer wouldn't let me open\n" +
			toNativeSeparators(encryptedFilePath));
		return false;
	}

	// Generate a file to store the decrypted data pre-extraction.
	QTemporaryFile temporaryFile;
	if (!temporaryFile.open()) {
		reportError("The computer wouldn't let me create a\n"
			"temporary file, which is needed for this to work.");
		return false;
	}

	// Decrypt the file.
	EncrypterReturn decryptStatus = this->encrypter->
		decryptFile(&encryptedFile, &temporaryFile);

	// Report issues.
	if (decryptStatus == ER_PASSWORD_WRONG) {
		reportError("The password was wrong for the file\n"
			+ toNativeSeparators(encryptedFilePath));
	}

	// Reset positions.
	encryptedFile.seek(0);
	temporaryFile.seek(0);

	// Unpack the file.
	ArchiverReturn archiveStatus = AR_ARCHIVE_ERROR;
	if (decryptStatus == ER_OK && temporaryFile.open()) {
		archiveStatus = this->archiver->
			extractFile(&temporaryFile, this->pathOut);
	}

	// Close out the files.
	encryptedFile.close();
	temporaryFile.close();

	// Indicate success/failure.
	return (decryptStatus == ER_OK && archiveStatus == AR_OK);
}

void CryptoThread::encryptFiles()
{
	// Create & open temporary files.
	QTemporaryFile archiveFile, encryptedFile;
	if (!archiveFile.open() || !encryptedFile.open()) {
		reportError("The computer wouldn't let me create a\n"
			"temporary file, which is needed for this to work.");
		return;
	}

	// Archive the file to a temporary location.
	ArchiverReturn archiveStatus = this->archiver->
		archiveFiles(this->pathIn, this->rootPathIn, &archiveFile);

	// Reset positions.
	archiveFile.seek(0);

	// If the archiving was OK, encrypt the contents.
	if (archiveStatus == AR_OK) {

		// Encrypt the contents.
		EncrypterReturn encryptStatus = this->encrypter->
			encryptFile(&archiveFile, &encryptedFile);

		// If the encryption was OK, make the file permanent.
		if (encryptStatus == ER_OK) {
			updateProgress(ENCRYPTION_MOVING_FILE, 1);
			if (this->renameTempFile(&encryptedFile, this->pathOut)) {
				reportComplete("Encryption Complete",
					"All done. Your encrypted file is found here:\n"
					+ toNativeSeparators(this->pathOut));
			}
		}

	// If the archiving was not OK, inform the user.
	}

	// Close temporary files.
	archiveFile.close();
	encryptedFile.close();
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

QString CryptoThread::toNativeSeparators(QString path)
{
	QString pathCopy = path;
	return pathCopy.replace("/", QDir::separator());
}

void CryptoThread::setupRun(bool isEncrypt, QStringList inputFiles,
	QString outputPath, QByteArray encryptionKey)
{
	// Missing critical parameters, abort.
	if (inputFiles.size() == 0) return;

	this->actionEncrypt = isEncrypt;
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
	return rootDirectory.absolutePath() + "/";
}

