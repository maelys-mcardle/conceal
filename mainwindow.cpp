#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	this->fileExtension = ".secure";
	this->cryptoThread = new CryptoThread(this);
	this->passwordDialog = new PasswordDialog(this);
	this->encryptDecryptDialog = new EncryptDecryptDialog(this);
	connect(this->cryptoThread, SIGNAL(finished()), this,
		SLOT(cryptoThreadDone()));
	connect(this->cryptoThread, SIGNAL(reportError(QString)), this,
		SLOT(cryptoError(QString)));
	connect(this->cryptoThread, SIGNAL(updateProgress(ProgressType,float)),
		this, SLOT(cryptoStatusUpdate(ProgressType,float)));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::cryptoThreadDone()
{
	// Re-enable the drops.
	this->setAcceptDrops(true);
}

void MainWindow::cryptoError(QString message)
{
	QMessageBox::warning(this, "There was a problem",
		message, QMessageBox::Ok);
}

void MainWindow::cryptoStatusUpdate(ProgressType type, float progress)
{

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
		QString outputPath = (decrypted) ?
			QFileDialog::getExistingDirectory(this,
				"Select where to place the decrypted files", ""):
			QFileDialog::getSaveFileName(this,
				"Choose where your encrypted file will go",
				"", "*" + this->fileExtension);

		// The user didn't specify anything. Abort.
		if (outputPath == "") return;

		// We're going to execute this. Prevent more drops.
		this->setAcceptDrops(false);

		// Setup the crypto thread with the details and execute.
		this->cryptoThread->setupRun(decrypted, pathList,
			outputPath, encryptionKey);
		this->cryptoThread->start();
	}
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
