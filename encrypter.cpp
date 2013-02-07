#include "encrypter.h"

Encrypter::Encrypter(QObject *parent) :
	QObject(parent)
{
	// Store the cryptographic key.
	this->progressUpdateInterval = 1048576;
}

EncrypterReturn Encrypter::setKey(QByteArray cryptoKey)
{
	// Open MCRYPT. Abort on fail.
	this->td = mcrypt_module_open(QByteArray(MCRYPT_RIJNDAEL_128).data(),
		NULL, QByteArray(MCRYPT_CBC).data(), NULL);
	if (this->td == MCRYPT_FAILED) {
		return ER_MCRYPT_OPEN_ERROR;
	}

	// Seed the pseudo-random number generator.
	QTime midnight(0, 0, 0);
	qsrand(midnight.secsTo(QTime::currentTime()));

	// Create the initialization vector.
	QByteArray IV(mcrypt_enc_get_iv_size(this->td), 0);
	for (int i = 0; i < IV.size(); i++) IV.replace(i, qrand());

	// Get the size of each block.
	this->encryptionBlockSize = mcrypt_enc_get_block_size(this->td);

	// Initialize. Return on fail.
	if (mcrypt_generic_init(this->td, cryptoKey.data(),
		cryptoKey.size(), IV.data()) < 0) {
		return ER_MCRYPT_INIT_ERROR;
	}

	return ER_OK;
}

Encrypter::~Encrypter()
{
	// Finish up.
	mcrypt_generic_end(this->td);
}


EncrypterReturn Encrypter::encryptFile(QFile *plaintext, QFile *ciphertext)
{
	// Abort on initialization failure.
	if (this->initializationStatus == ER_MCRYPT_OPEN_ERROR ||
		this->initializationStatus == ER_MCRYPT_INIT_ERROR)
		return this->initializationStatus;

	// The IV will not be communicated, so prepend a sacrificial block.
	QByteArray firstBlock = generateRandomBlock();
	mcrypt_generic (this->td, firstBlock.data(), firstBlock.size());
	ciphertext->write(firstBlock.data(), firstBlock.size());

	// Load in a challenge for the encryption.
	QByteArray challenge = generateChallenge();
	mcrypt_generic(this->td, challenge.data(), challenge.size());
	ciphertext->write(challenge.data(), challenge.size());

	// Encrypt the file.
	plaintext->seek(0);
	qint64 lastProgressUpdate = 0;
	while (!plaintext->atEnd()) {

		// Update the status for the UI.
		if (plaintext->pos() > lastProgressUpdate + this->progressUpdateInterval) {
			updateProgress(ARCHIVE_TO_CIPHERTEXT,
				(float) plaintext->pos() / (float) plaintext->size());
			lastProgressUpdate = plaintext->pos();
		}

		// Read the data in blocks.
		QByteArray block(this->encryptionBlockSize, 0);
		int dataRead = plaintext->read(block.data(), block.size());
		if (dataRead == 0) break;

		// Encrypt the data and write it.
		mcrypt_generic(this->td, block.data(), dataRead);
		ciphertext->write(block.data(), dataRead);
	}

	// send a final status update.
	updateProgress(ARCHIVE_TO_CIPHERTEXT, 1);
	return ER_OK;
}

QByteArray Encrypter::generateRandomBlock()
{
	QByteArray data;
	for (int i = 0; i < this->encryptionBlockSize; i++)
		data.append(qrand());
	return data;
}

QByteArray Encrypter::generateChallenge()
{
	// Create the array with the random block.
	QByteArray data = generateRandomBlock();

	// Create a simple challenge to validate the data.
	quint32 valueA = qrand(), valueB = qrand();
	quint32 valueXor = (valueA ^ valueB);

	// Load in the challenge.
	memcpy(&data.data()[0], &valueA, sizeof(quint32));
	memcpy(&data.data()[sizeof(quint32)], &valueB, sizeof(quint32));
	memcpy(&data.data()[sizeof(quint32)*2],&valueXor, sizeof(quint32));

	// Return the data set.
	return data;
}

bool Encrypter::validateChallenge(QByteArray data)
{
	// Extract the challenge values.
	quint32 valueA, valueB, valueXor;
	memcpy(&valueA, &data.data()[0], sizeof(quint32));
	memcpy(&valueB, &data.data()[sizeof(quint32)], sizeof(quint32));
	memcpy(&valueXor, &data.data()[sizeof(quint32)*2], sizeof(quint32));

	// Validate the values.
	if ((valueXor ^ valueB) != valueA) return false;
	return true;
}

EncrypterReturn Encrypter::decryptFile(QFile *ciphertext, QFile *plaintext)
{
	// Abort on initialization failure.
	if (this->initializationStatus == ER_MCRYPT_OPEN_ERROR ||
		this->initializationStatus == ER_MCRYPT_INIT_ERROR)
		return this->initializationStatus;

	// Go to the start of the file.
	QDataStream stream(ciphertext);
	ciphertext->seek(0);

	// Decrypt the file.
	qint64 lastProgressUpdate = 0;
	for (qint64 currentBlock = 0; !ciphertext->atEnd(); currentBlock++) {

		// Update the status for the UI.
		if (ciphertext->pos() > lastProgressUpdate + this->progressUpdateInterval) {
			updateProgress(CIPHERTEXT_TO_ARCHIVE,
				(float) ciphertext->pos() / (float) ciphertext->size());
			lastProgressUpdate = ciphertext->pos();
		}

		// Read the data in blocks.
		QByteArray block(this->encryptionBlockSize, 0);
		int dataRead = stream.readRawData(block.data(), block.size());

		// Decrypt the data.
		mdecrypt_generic (this->td, block.data(), dataRead);

		// Skip the first block, which was mangled by the random IV.
		if (currentBlock == 1 && validateChallenge(block) == false)
			return ER_PASSWORD_WRONG;

		// Write the data back. Skip the first two blocks. The first
		// was mangled by the IV, the second is the challenge.
		if (currentBlock > 1)
			plaintext->write(block.data(), dataRead);
	}

	// Report success.
	return ER_OK;
}
