/*
 * Copyright (C) 2003-2005  Justin Karneges <justin@affinix.com>
 * Copyright (C) 2004,2005,2007  Brad Hards <bradh@frogmouth.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "qca_basic.h"

#include <QtCore>
#include "qcaprovider.h"

namespace QCA {

//----------------------------------------------------------------------------
// Random
//----------------------------------------------------------------------------
Random::Random(const QString &provider)
:Algorithm("random", provider)
{
}

uchar Random::nextByte()
{
	return (uchar)(nextBytes(1)[0]);
}

QSecureArray Random::nextBytes(int size)
{
	return static_cast<RandomContext *>(context())->nextBytes(size);
}

uchar Random::randomChar()
{
	return globalRNG().nextByte();
}

uint Random::randomInt()
{
	QSecureArray a = globalRNG().nextBytes(sizeof(int));
	uint x;
	memcpy(&x, a.data(), a.size());
	return x;
}

QSecureArray Random::randomArray(int size)
{
	return globalRNG().nextBytes(size);
}

//----------------------------------------------------------------------------
// Hash
//----------------------------------------------------------------------------
Hash::Hash(const QString &type, const QString &provider)
:Algorithm(type, provider)
{
}

void Hash::clear()
{
	static_cast<HashContext *>(context())->clear();
}

void Hash::update(const QSecureArray &a)
{
	static_cast<HashContext *>(context())->update(a);
}

void Hash::update(const QByteArray &a)
{
	update( QSecureArray( a ) );
}

void Hash::update(const char *data, int len)
{
	if ( len < 0 )
		len = qstrlen( data );
	if ( 0 == len )
		return;

	update(QByteArray::fromRawData(data, len));
}

// Reworked from KMD5, from KDE's kdelibs
void Hash::update(QIODevice &file)
{
	char buffer[1024];
	int len;

	while ((len=file.read(reinterpret_cast<char*>(buffer), sizeof(buffer))) > 0)
		update(buffer, len);
}

QSecureArray Hash::final()
{
	return static_cast<HashContext *>(context())->final();
}

QSecureArray Hash::hash(const QSecureArray &a)
{
	return process(a);
}

QString Hash::hashToString(const QSecureArray &a)
{
	return arrayToHex(hash(a));
}

//----------------------------------------------------------------------------
// Cipher
//----------------------------------------------------------------------------
class Cipher::Private
{
public:
	Direction dir;
	SymmetricKey key;
	InitializationVector iv;

	bool ok, done;
};

Cipher::Cipher( const QString &type, Mode m, Padding pad,
	Direction dir, const SymmetricKey &key,
	const InitializationVector &iv,
	const QString &provider )
:Algorithm(withAlgorithms( type, m, pad ), provider)
{
	d = new Private;
	if(!key.isEmpty())
		setup(dir, key, iv);
}

Cipher::Cipher(const Cipher &from)
:Algorithm(from), Filter(from)
{
	d = new Private(*from.d);
}

Cipher::~Cipher()
{
	delete d;
}

Cipher & Cipher::operator=(const Cipher &from)
{
	Algorithm::operator=(from);
	*d = *from.d;
	return *this;
}

KeyLength Cipher::keyLength() const
{
	return static_cast<const CipherContext *>(context())->keyLength();
}

bool Cipher::validKeyLength(int n) const
{
	KeyLength len = keyLength();
	return ((n >= len.minimum()) && (n <= len.maximum()) && (n % len.multiple() == 0));
}

unsigned int Cipher::blockSize() const
{
	return static_cast<const CipherContext *>(context())->blockSize();
}

void Cipher::clear()
{
	d->done = false;
	static_cast<CipherContext *>(context())->setup(d->dir, d->key, d->iv);
}

QSecureArray Cipher::update(const QSecureArray &a)
{
	QSecureArray out;
	if(d->done)
		return out;
	d->ok = static_cast<CipherContext *>(context())->update(a, &out);
	return out;
}

QSecureArray Cipher::final()
{
	QSecureArray out;
	if(d->done)
		return out;
	d->done = true;
	d->ok = static_cast<CipherContext *>(context())->final(&out);
	return out;
}

bool Cipher::ok() const
{
	return d->ok;
}

void Cipher::setup(Direction dir, const SymmetricKey &key, const InitializationVector &iv)
{
	d->dir = dir;
	d->key = key;
	d->iv = iv;
	clear();
}

QString Cipher::withAlgorithms(const QString &cipherType, Mode modeType, Padding paddingType)
{
	QString mode;
	switch(modeType) {
	case CBC:
		mode = "cbc";
		break;
	case CFB:
		mode = "cfb";
		break;
	case OFB:
		mode = "ofb";
		break;
	case ECB:
		mode = "ecb";
		break;
	default:
		abort();
	}

	// do the default
	if(paddingType == DefaultPadding)
	{
		// logic from Botan
		if(modeType == CBC)
			paddingType = PKCS7;
		else
			paddingType = NoPadding;
	}

	QString pad;
	if(paddingType == NoPadding)
		pad = "";
	else
		pad = "pkcs7";

	QString result = cipherType + '-' + mode;
	if(!pad.isEmpty())
		result += QString("-") + pad;

	return result;
}

//----------------------------------------------------------------------------
// MessageAuthenticationCode
//----------------------------------------------------------------------------
class MessageAuthenticationCode::Private
{
public:
	SymmetricKey key;

	bool done;
	QSecureArray buf;
};


MessageAuthenticationCode::MessageAuthenticationCode(const QString &type,
						     const SymmetricKey &key,
						     const QString &provider)
  :Algorithm(type, provider)
{
	d = new Private;
	setup(key);
}

MessageAuthenticationCode::MessageAuthenticationCode(const MessageAuthenticationCode &from)
:Algorithm(from), BufferedComputation(from)
{
	d = new Private(*from.d);
}

MessageAuthenticationCode::~MessageAuthenticationCode()
{
	delete d;
}

MessageAuthenticationCode & MessageAuthenticationCode::operator=(const MessageAuthenticationCode &from)
{
	Algorithm::operator=(from);
	*d = *from.d;
	return *this;
}

KeyLength MessageAuthenticationCode::keyLength() const
{
	return static_cast<const MACContext *>(context())->keyLength();
}

bool MessageAuthenticationCode::validKeyLength(int n) const
{
	KeyLength len = keyLength();
	return ((n >= len.minimum()) && (n <= len.maximum()) && (n % len.multiple() == 0));
}

void MessageAuthenticationCode::clear()
{
	d->done = false;
	static_cast<MACContext *>(context())->setup(d->key);
}

void MessageAuthenticationCode::update(const QSecureArray &a)
{
	if(d->done)
		return;
	static_cast<MACContext *>(context())->update(a);
}

QSecureArray MessageAuthenticationCode::final()
{
	if(!d->done)
	{
		d->done = true;
		static_cast<MACContext *>(context())->final(&d->buf);
	}
	return d->buf;
}

void MessageAuthenticationCode::setup(const SymmetricKey &key)
{
	d->key = key;
	clear();
}

//----------------------------------------------------------------------------
// Key Derivation Function
//----------------------------------------------------------------------------
KeyDerivationFunction::KeyDerivationFunction(const QString &type, const QString &provider)
:Algorithm(type, provider)
{
}

KeyDerivationFunction::KeyDerivationFunction(const KeyDerivationFunction &from)
:Algorithm(from)
{
}

KeyDerivationFunction::~KeyDerivationFunction()
{
}

KeyDerivationFunction & KeyDerivationFunction::operator=(const KeyDerivationFunction &from)
{
	Algorithm::operator=(from);
	return *this;
}

SymmetricKey KeyDerivationFunction::makeKey(const QSecureArray &secret, const InitializationVector &salt, unsigned int keyLength, unsigned int iterationCount)
{
	return static_cast<KDFContext *>(context())->makeKey(secret, salt, keyLength, iterationCount);
}

QString KeyDerivationFunction::withAlgorithm(const QString &kdfType, const QString &algType)
{
	return (kdfType + '(' + algType + ')');
}

}
