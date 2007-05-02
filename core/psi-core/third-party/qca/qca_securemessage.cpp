/*
 * Copyright (C) 2003-2005  Justin Karneges <justin@affinix.com>
 * Copyright (C) 2004,2005  Brad Hards <bradh@frogmouth.net>
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

#include "qca_securemessage.h"

#include <QtCore>
#include "qcaprovider.h"

namespace QCA {

Provider::Context *getContext(const QString &type, const QString &provider);

//----------------------------------------------------------------------------
// SecureMessageKey
//----------------------------------------------------------------------------
class SecureMessageKey::Private : public QSharedData
{
public:
	SecureMessageKey::Type type;
	PGPKey pgp_pub, pgp_sec;
	CertificateChain cert_pub;
	PrivateKey cert_sec;

	Private()
	{
		type = SecureMessageKey::None;
	}

	// set the proper type, and reset the opposite data structures if needed
	void ensureType(SecureMessageKey::Type t)
	{
		// if we were non-null and changed, we may need to reset some things
		if(type != SecureMessageKey::None && t != type)
		{
			if(type == SecureMessageKey::X509)
			{
				cert_pub = CertificateChain();
				cert_sec = PrivateKey();
			}
			else if(type == SecureMessageKey::PGP)
			{
				pgp_pub = PGPKey();
				pgp_sec = PGPKey();
			}
		}
		type = t;
	}
};

SecureMessageKey::SecureMessageKey()
:d(new Private)
{
}

SecureMessageKey::SecureMessageKey(const SecureMessageKey &from)
:d(from.d)
{
}

SecureMessageKey::~SecureMessageKey()
{
}

SecureMessageKey & SecureMessageKey::operator=(const SecureMessageKey &from)
{
	d = from.d;
	return *this;
}

bool SecureMessageKey::isNull() const
{
	return (d->type == None);
}

SecureMessageKey::Type SecureMessageKey::type() const
{
	return d->type;
}

PGPKey SecureMessageKey::pgpPublicKey() const
{
	return d->pgp_pub;
}

PGPKey SecureMessageKey::pgpSecretKey() const
{
	return d->pgp_sec;
}

void SecureMessageKey::setPGPPublicKey(const PGPKey &pub)
{
	d->ensureType(SecureMessageKey::PGP);
	d->pgp_pub = pub;
}

void SecureMessageKey::setPGPSecretKey(const PGPKey &sec)
{
	d->ensureType(SecureMessageKey::PGP);
	d->pgp_sec = sec;
}

CertificateChain SecureMessageKey::x509CertificateChain() const
{
	return d->cert_pub;
}

PrivateKey SecureMessageKey::x509PrivateKey() const
{
	return d->cert_sec;
}

void SecureMessageKey::setX509CertificateChain(const CertificateChain &c)
{
	d->ensureType(SecureMessageKey::X509);
	d->cert_pub = c;
}

void SecureMessageKey::setX509PrivateKey(const PrivateKey &k)
{
	d->ensureType(SecureMessageKey::X509);
	d->cert_sec = k;
}

bool SecureMessageKey::havePrivate() const
{
	if(d->type == SecureMessageKey::PGP && !d->pgp_sec.isNull())
		return true;
	else if(d->type == SecureMessageKey::X509 && !d->cert_sec.isNull())
		return true;
	return false;
}

QString SecureMessageKey::name() const
{
	if(d->type == SecureMessageKey::PGP && !d->pgp_pub.isNull())
		return d->pgp_pub.primaryUserId();
	else if(d->type == SecureMessageKey::X509 && !d->cert_pub.isEmpty())
		return d->cert_pub.primary().commonName();
	else
		return QString();
}

//----------------------------------------------------------------------------
// SecureMessageSignature
//----------------------------------------------------------------------------
class SecureMessageSignature::Private : public QSharedData
{
public:
	SecureMessageSignature::IdentityResult r;
	Validity v;
	SecureMessageKey key;
	QDateTime ts;

	Private()
	{
		r = SecureMessageSignature::NoKey;
		v = ErrorValidityUnknown;
	}
};

SecureMessageSignature::SecureMessageSignature()
:d(new Private)
{
}

SecureMessageSignature::SecureMessageSignature(IdentityResult r, Validity v, const SecureMessageKey &key, const QDateTime &ts)
:d(new Private)
{
	d->r = r;
	d->v = v;
	d->key = key;
	d->ts = ts;
}

SecureMessageSignature::SecureMessageSignature(const SecureMessageSignature &from)
:d(from.d)
{
}

SecureMessageSignature::~SecureMessageSignature()
{
}

SecureMessageSignature & SecureMessageSignature::operator=(const SecureMessageSignature &from)
{
	d = from.d;
	return *this;
}

SecureMessageSignature::IdentityResult SecureMessageSignature::identityResult() const
{
	return d->r;
}

Validity SecureMessageSignature::keyValidity() const
{
	return d->v;
}

SecureMessageKey SecureMessageSignature::key() const
{
	return d->key;
}

QDateTime SecureMessageSignature::timestamp() const
{
	return d->ts;
}

//----------------------------------------------------------------------------
// SecureMessage
//----------------------------------------------------------------------------
enum ResetMode
{
        ResetSession        = 0,
        ResetSessionAndData = 1,
        ResetAll            = 2
};

class SecureMessage::Private : public QObject
{
	Q_OBJECT
public:
	SecureMessage *q;
	MessageContext *c;
	SecureMessageSystem *system;

	bool bundleSigner, smime;
	SecureMessage::Format format;
	SecureMessageKeyList to;
	SecureMessageKeyList from;

	QByteArray in;
	bool success;
	SecureMessage::Error errorCode;
	QByteArray detachedSig;
	QString hashName;
	SecureMessageSignatureList signers;

	Private(SecureMessage *_q)
	{
		q = _q;
		c = 0;
		system = 0;
		reset(ResetAll);
	}

	void init()
	{
		connect(c, SIGNAL(updated()), SLOT(updated()));
	}

	void reset(ResetMode mode)
	{
		if(c)
			c->reset();

		if(mode >= ResetSessionAndData)
		{
			in.clear();
			success = false;
			errorCode = SecureMessage::ErrorUnknown;
			detachedSig.clear();
			hashName = QString();
			signers.clear();
		}

		if(mode >= ResetAll)
		{
			bundleSigner = true;
			format = SecureMessage::Binary;
			to.clear();
			from.clear();
		}
	}

public slots:
	void updated()
	{
		bool sig_read = false;
		bool sig_done = false;
		{
			QByteArray a = c->read();
			if(!a.isEmpty())
			{
				sig_read = true;
				in.append(a);
			}
		}

		if(c->finished())
		{
			sig_done = true;

			success = c->success();
			errorCode = c->errorCode();
			if(success)
			{
				detachedSig = c->signature();
				hashName = c->hashName();
				signers = c->signers();
			}
			reset(ResetSession);
		}

		if(sig_read)
			QTimer::singleShot(0, q, SIGNAL(readyRead()));
		if(sig_done)
			QTimer::singleShot(0, q, SIGNAL(finished()));
	}
};

SecureMessage::SecureMessage(SecureMessageSystem *system)
{
	d = new Private(this);
	d->system = system;
	d->c = static_cast<SMSContext *>(d->system->context())->createMessage();
	change(d->c);
	d->init();
}

SecureMessage::~SecureMessage()
{
	delete d;
}

SecureMessage::Type SecureMessage::type() const
{
	return d->c->type();
}

bool SecureMessage::canSignMultiple() const
{
	return d->c->canSignMultiple();
}

bool SecureMessage::canClearsign() const
{
	return (type() == OpenPGP);
}

bool SecureMessage::canSignAndEncrypt() const
{
	return (type() == OpenPGP);
}

void SecureMessage::reset()
{
	d->reset(ResetAll);
}

void SecureMessage::setEnableBundleSigner(bool b)
{
	d->bundleSigner = b;
}

void SecureMessage::setEnableSMIMEAttributes(bool b)
{
	d->smime = b;
}

void SecureMessage::setFormat(Format f)
{
	d->format = f;
}

void SecureMessage::setRecipient(const SecureMessageKey &key)
{
	d->to = SecureMessageKeyList() << key;
}

void SecureMessage::setRecipients(const SecureMessageKeyList &keys)
{
	d->to = keys;
}

void SecureMessage::setSigner(const SecureMessageKey &key)
{
	d->from = SecureMessageKeyList() << key;
}

void SecureMessage::setSigners(const SecureMessageKeyList &keys)
{
	d->from = keys;
}

void SecureMessage::startEncrypt()
{
	d->reset(ResetSessionAndData);
	d->c->setupEncrypt(d->to);
	d->c->start(d->format, MessageContext::Encrypt);
}

void SecureMessage::startDecrypt()
{
	d->reset(ResetSessionAndData);
	d->c->start(d->format, MessageContext::Decrypt);
}

void SecureMessage::startSign(SignMode m)
{
	d->reset(ResetSessionAndData);
	d->c->setupSign(d->from, m, d->bundleSigner, d->smime);
	d->c->start(d->format, MessageContext::Sign);
}

void SecureMessage::startVerify(const QByteArray &sig)
{
	d->reset(ResetSessionAndData);
	if(!sig.isEmpty())
		d->c->setupVerify(sig);
	d->c->start(d->format, MessageContext::Verify);
}

void SecureMessage::startSignAndEncrypt()
{
	d->reset(ResetSessionAndData);
	d->c->setupEncrypt(d->to);
	d->c->setupSign(d->from, Message, d->bundleSigner, d->smime);
	d->c->start(d->format, MessageContext::SignAndEncrypt);
}

void SecureMessage::update(const QByteArray &in)
{
	d->c->update(in);
}

QByteArray SecureMessage::read()
{
	QByteArray a = d->in;
	d->in.clear();
	return a;
}

int SecureMessage::bytesAvailable() const
{
	return d->in.size();
}

void SecureMessage::end()
{
	d->c->end();
}

bool SecureMessage::waitForFinished(int msecs)
{
	d->c->waitForFinished(msecs);
	d->updated();
	return d->success;
}

bool SecureMessage::success() const
{
	return d->success;
}

SecureMessage::Error SecureMessage::errorCode() const
{
	return d->errorCode;
}

QByteArray SecureMessage::signature() const
{
	return d->detachedSig;
}

QString SecureMessage::hashName() const
{
	return d->hashName;
}

bool SecureMessage::wasSigned() const
{
	return !d->signers.isEmpty();
}

bool SecureMessage::verifySuccess() const
{
	// if we're not done or there were no signers, then return false
	if(!d->success || d->signers.isEmpty())
		return false;

	// make sure all signers have a valid signature
	for(int n = 0; n < d->signers.count(); ++n)
	{
		if(d->signers[n].identityResult() != SecureMessageSignature::Valid)
			return false;
	}
	return true;
}

SecureMessageSignature SecureMessage::signer() const
{
	if(d->signers.isEmpty())
		return SecureMessageSignature();

	return d->signers.first();
}

SecureMessageSignatureList SecureMessage::signers() const
{
	return d->signers;
}

QString SecureMessage::diagnosticText() const
{
	// TODO
	return QString();
}

//----------------------------------------------------------------------------
// SecureMessageSystem
//----------------------------------------------------------------------------
SecureMessageSystem::SecureMessageSystem(QObject *parent, const QString &type, const QString &provider)
:QObject(parent), Algorithm(type, provider)
{
}

SecureMessageSystem::~SecureMessageSystem()
{
}

//----------------------------------------------------------------------------
// OpenPGP
//----------------------------------------------------------------------------
OpenPGP::OpenPGP(QObject *parent, const QString &provider)
:SecureMessageSystem(parent, "openpgp", provider)
{
}

OpenPGP::~OpenPGP()
{
}

//----------------------------------------------------------------------------
// CMS
//----------------------------------------------------------------------------
CMS::CMS(QObject *parent, const QString &provider)
:SecureMessageSystem(parent, "cms", provider)
{
}

CMS::~CMS()
{
}

void CMS::setTrustedCertificates(const CertificateCollection &trusted)
{
	static_cast<SMSContext *>(context())->setTrustedCertificates(trusted);
}

void CMS::setPrivateKeys(const SecureMessageKeyList &keys)
{
	static_cast<SMSContext *>(context())->setPrivateKeys(keys);
}

}

#include "qca_securemessage.moc"
