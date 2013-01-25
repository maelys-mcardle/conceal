#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	this->fileExtension = "secure";
	this->cryptoThread = new CryptoThread(this);
	this->passwordDialog = new PasswordDialog(this);
	this->encryptDecryptDialog = new EncryptDecryptDialog(this);
	this->licenseDialog = new LicenseDialog(this);

	qRegisterMetaType<ProgressType>("ProgressType");

	connect(this->cryptoThread, SIGNAL(finished()), this,
		SLOT(cryptoThreadDone()));
	connect(this->cryptoThread, SIGNAL(terminated()), this,
		SLOT(cryptoThreadDone()));
	connect(this->cryptoThread, SIGNAL(reportError(QString)), this,
		SLOT(cryptoError(QString)));
	connect(this->cryptoThread, SIGNAL(reportComplete(QString)), this,
		SLOT(cryptoComplete(QString)));
	connect(this->cryptoThread, SIGNAL(updateProgress(ProgressType,float)),
		this, SLOT(cryptoStatusUpdate(ProgressType,float)));

	this->qmlRootObject = ui->declarativeView->rootObject();
	this->ciphertextImage = this->qmlRootObject->findChild<QObject*>("ciphertextImage");
	this->plaintextImage = this->qmlRootObject->findChild<QObject*>("plaintextImage");
	this->archiveImage = this->qmlRootObject->findChild<QObject*>("archiveImage");
	this->progressText = this->qmlRootObject->findChild<QObject*>("progressText");

	ui->declarativeView->rootContext()->setContextProperty("mainWindow", this);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::cryptoThreadDone()
{
	// Re-enable the drops.
	this->setAcceptDrops(true);

	// Restore the GUI.
	qmlRootObject->setProperty("state", "Default");
}

void MainWindow::cancelCrypto()
{
	int response = QMessageBox::question(this, "Warning",
		"Are you sure you want to cancel? I'll continue\n"
		"working in the background while you decide.",
		QMessageBox::Yes | QMessageBox::No);

	if (response == QMessageBox::Yes)
		this->cryptoThread->terminate();
}

void MainWindow::cryptoComplete(QString message)
{
	QMessageBox::information(this, "I'm finished",
		message, QMessageBox::Ok);
}

void MainWindow::cryptoError(QString message)
{
	QMessageBox::warning(this, "There was a problem",
		message, QMessageBox::Ok);
}

void MainWindow::cryptoStatusUpdate(ProgressType type, float progress)
{
	QString percentProgress;
	percentProgress.sprintf(" (%.2f%%)", progress * 100);

	if (type == PLAINTEXT_TO_ARCHIVE) {
		this->plaintextImage->setProperty("opacity", 1-progress);
		this->archiveImage->setProperty("opacity", progress);
		this->ciphertextImage->setProperty("opacity", 0);
		this->progressText->setProperty("text",
			"Packaging your files" + percentProgress);
	} else if (type == ARCHIVE_TO_CIPHERTEXT) {
		this->plaintextImage->setProperty("opacity", 0);
		this->archiveImage->setProperty("opacity", 1-progress);
		this->ciphertextImage->setProperty("opacity", progress);
		this->progressText->setProperty("text",
			"Encrypting your files" + percentProgress);
	} else if (type == CIPHERTEXT_TO_ARCHIVE) {
		this->plaintextImage->setProperty("opacity", 0);
		this->archiveImage->setProperty("opacity", progress);
		this->ciphertextImage->setProperty("opacity", 1-progress);
		this->progressText->setProperty("text",
			"Decrypting your files" + percentProgress);
	} else if (type == ARCHIVE_TO_PLAINTEXT) {
		this->plaintextImage->setProperty("opacity", progress);
		this->archiveImage->setProperty("opacity", 1-progress);
		this->ciphertextImage->setProperty("opacity", 0);
		this->progressText->setProperty("text",
			"Unpacking your files" + percentProgress);
	} else if (type == ENCRYPTION_MOVING_FILE) {
		this->progressText->setProperty("text", "Finishing Up");
	} else if (type == DECRYPTION_MOVING_FILE) {
		this->progressText->setProperty("text", "Moving files");
	}
}

void MainWindow::showLicense()
{
	this->licenseDialog->show();
}

void MainWindow::dropEvent(QDropEvent *event)
{
	const QMimeData* mimeData = event->mimeData();

	// check for our needed mime type, here a file or a list of files
	if (mimeData->hasUrls())
	{
		QStringList pathList;
		QList<QUrl> urlList = mimeData->urls();

		// Extract the local paths of the files.
		for (int i = 0; i < urlList.size() && i < 32; ++i)
			pathList.append(urlList.at(i).toLocalFile());

		// Ask the user to enter a password, abort on cancel.
		if (this->passwordDialog->exec() == QDialog::Rejected) return;
		QByteArray encryptionKey = this->passwordDialog->getHash();

		// Determine if the files are being encrypted or decrypted.
		bool ok;
		bool decrypted = this->isDecrypt(pathList, &ok);
		if (ok == false) return;

		// If it's decryption, as the output directory. Otherwise
		// ask where to save the encrypted file.
		QString outputPath = getOutputPath(decrypted);

		// The user didn't specify anything. Abort.
		if (outputPath == "") return;

		// We're going to execute this. Prevent more drops.
		this->setAcceptDrops(false);

		// Change the GUI to show the processing mode.
		if (decrypted) qmlRootObject->setProperty("state", "Decrypt");
		else qmlRootObject->setProperty("state", "Encrypt");

		// Setup the crypto thread with the details and execute.
		this->cryptoThread->setupRun(decrypted, pathList,
			outputPath, encryptionKey);
		this->cryptoThread->start();
	}
}

QString MainWindow::getOutputPath(bool decrypt)
{
	// Ask the user for a path.
	QString path = (decrypt) ?
		QFileDialog::getExistingDirectory(this,
			"Select where to place the decrypted files", ""):
		QFileDialog::getSaveFileName(this,
			"Choose where your encrypted file will go",
			"", "Encrypted Files (*." + this->fileExtension + ")");

	// If it's empty, abort.
	if (path.isEmpty()) return path;

	// If it was a file, and it already exists after adding a missing
	// file extension, warn the user.
	if (!decrypt && !path.endsWith(this->fileExtension)) {
		path.append("." + this->fileExtension);
		if (QFileInfo(path).exists() &&
			QMessageBox::question(this, "Overwrite file?",
			"The file " + QFileInfo(path).fileName() + " already exists.\n"
			"Do you want to overwrite it?", QMessageBox::Yes |
			QMessageBox::No) == QMessageBox::No) {
			path = this->getOutputPath(decrypt);
		}
	}

	// If it's a directory, and not empty, warn the user.
	if (decrypt && QDir(path).entryList(QDir::NoDotAndDotDot |
		QDir::AllEntries).size() > 0) {
		if (QMessageBox::question(this, "The directory has stuff in it",
			"The directory you selected isn't empty. This means\n"
			"that some stuff inside could get over-written. Do you\n"
			"want to pick a different directory?", QMessageBox::Yes |
			QMessageBox::No) == QMessageBox::Yes) {
			path = this->getOutputPath(decrypt);
		}
	}

	return path;
}

bool MainWindow::isDecrypt(QStringList files, bool *ok)
{
	// Assume all checks out by default.
	*ok = true;

	// Get the number of encrypted and non encrypted files.
	int encrypted = 0, decrypted = 0;
	for (int i = 0; i < files.size(); i++) {
		if (files.at(i).endsWith(this->fileExtension)) encrypted++;
		else decrypted++;
	}

	// If it's a single encrypted file, assume decryption.
	if (encrypted == 1 && decrypted == 0) return true;

	// If it's all decrypted, assume encryption.
	if (encrypted == 0 && decrypted > 0) return false;

	// If it's anything else, ask the user. Abort on cancel.
	if (this->encryptDecryptDialog->exec() == QDialog::Rejected) {
		*ok = false;
		return false;
	}

	// Return what the user specified.
	return this->encryptDecryptDialog->isDecrypt();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
	event->accept();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
	event->acceptProposedAction();
}
