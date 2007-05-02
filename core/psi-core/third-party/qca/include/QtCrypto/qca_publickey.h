/*
 * qca_publickey.h - Qt Cryptographic Architecture
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

/**
   \file qca_publickey.h

   Header file for PublicKey and PrivateKey related classes

   \note You should not use this header directly from an
   application. You should just use <tt> \#include \<QtCrypto>
   </tt> instead.
*/

#ifndef QCA_PUBLICKEY_H
#define QCA_PUBLICKEY_H

#include <QObject>
#include "qca_core.h"

namespace QCA
{
	class PublicKey;
	class PrivateKey;
	class KeyGenerator;
	class RSAPublicKey;
	class RSAPrivateKey;
	class DSAPublicKey;
	class DSAPrivateKey;
	class DHPublicKey;
	class DHPrivateKey;

	/**
	   Encryption algorithms
	*/
	enum EncryptionAlgorithm
	{
		EME_PKCS1v15,  ///< Block type 2 (PKCS#1, Version 1.5)
		EME_PKCS1_OAEP ///< Optimal asymmetric encryption padding (PKCS#1, Version 2.0)
	};

	/**
	   Signature algorithm variants
	*/
	enum SignatureAlgorithm
	{
		SignatureUnknown, ///< Unknown signing algorithm
		EMSA1_SHA1,  ///< SHA1, with EMSA1 (IEEE1363-2000) encoding (this is the usual DSA algorithm - FIPS186)
		EMSA3_SHA1,  ///< SHA1, with EMSA3 (ie PKCS#1 Version 1.5) encoding
		EMSA3_MD5,   ///< MD5, with EMSA3 (ie PKCS#1 Version 1.5) encoding (this is the usual RSA algorithm)
		EMSA3_MD2,   ///< MD2, with EMSA3 (ie PKCS#1 Version 1.5) encoding
		EMSA3_RIPEMD160, ///< RIPEMD160, with EMSA3 (ie PKCS#1 Version 1.5) encoding
		EMSA3_Raw ///< EMSA3 without computing a message digest or a DigestInfo encoding (identical to PKCS#11's CKM_RSA_PKCS mechanism)
	};

	/**
	   Signature formats (DSA only)
	*/
	enum SignatureFormat
	{
		DefaultFormat, ///< For DSA, this is the same as IEEE_1363
		IEEE_1363,     ///< 40-byte format from IEEE 1363 (Botan/.NET)
		DERSequence    ///< Signature wrapped in DER formatting (OpenSSL/Java)
	};

	/**
	   Password-based encryption
	*/
	enum PBEAlgorithm
	{
		PBEDefault,           ///< Use modern default (same as PBES2_TripleDES_SHA1)
		PBES2_DES_SHA1,       ///< PKCS#5 v2.0 DES/CBC,SHA1
		PBES2_TripleDES_SHA1, ///< PKCS#5 v2.0 TripleDES/CBC,SHA1
		PBES2_AES128_SHA1,    ///< PKCS#5 v2.0 AES-128/CBC,SHA1
		PBES2_AES192_SHA1,    ///< PKCS#5 v2.0 AES-192/CBC,SHA1
		PBES2_AES256_SHA1     ///< PKCS#5 v2.0 AES-256/CBC,SHA1
	};

	/**
	   Return value from a format conversion

	   Note that if you are checking for any result other than ConvertGood,
	   then you may be introducing a provider specific dependency.
	*/
	enum ConvertResult
	{
		ConvertGood,      ///< Conversion succeeded, results should be valid
		ErrorDecode,      ///< General failure in the decode stage
		ErrorPassphrase,  ///< Failure because of incorrect pass phrase
		ErrorFile         ///< Failure because of incorrect file
	};

	/**
	   Well known discrete logarithm group sets
	*/
	enum DLGroupSet
	{
		DSA_512,
		DSA_768,
		DSA_1024,
		IETF_768,
		IETF_1024,
		IETF_1536,
		IETF_2048,
		IETF_3072,
		IETF_4096
	};

	/**
	   \class DLGroup qca_publickey.h QtCrypto

	   A discrete logarithm group
	*/
	class QCA_EXPORT DLGroup
	{
	public:
		DLGroup();

		/**
		   Construct a discrete logarithm group from raw parameters

		   \param p
		   \param q
		   \param g
		*/
		DLGroup(const QBigInteger &p, const QBigInteger &q, const QBigInteger &g);

		/**
		   Construct a discrete logarithm group from raw parameters

		   \param p
		   \param g
		*/
		DLGroup(const QBigInteger &p, const QBigInteger &g);

		/**
		   Standard copy constructor
		*/
		DLGroup(const DLGroup &from);
		~DLGroup();

		/**
		   Standard assignment operator

		   \param from the DLGroup to copy from
		*/
		DLGroup & operator=(const DLGroup &from);

		/**
		   Provide a list of the supported group sets

		   \param provider the provider to report which group sets are available. If not
		   specified, all providers will be checked
		*/
		static QList<DLGroupSet> supportedGroupSets(const QString &provider = QString());

		/**
		   Test if the group is empty
		*/
		bool isNull() const;

		/**
		   Provide the p component of the group
		*/
		QBigInteger p() const;

		/**
		   Provide the q component of the group
		*/
		QBigInteger q() const;

		/**
		   Provide the g component of the group
		*/
		QBigInteger g() const;

	private:
		class Private;
		Private *d;
	};

	/**
	   \class PKey qca_publickey.h QtCrypto

	   General superclass for public (PublicKey) and private (PrivateKey) keys
	   used with asymmetric encryption techniques.
	*/
	class QCA_EXPORT PKey : public Algorithm
	{
	public:
		/**
		   Types of public key cryptography keys supported by QCA
		*/
		enum Type { 
			RSA, ///< RSA key
			DSA, ///< DSA key
			DH   ///< Diffie Hellman key
		};

		PKey();

		/**
		   Standard copy constructor

		   \param from the key to copy from
		*/
		PKey(const PKey &from);
		~PKey();

		/**
		   Standard assignment operator

		   \param from the PKey to copy from
		*/
		PKey & operator=(const PKey &from);

		/**
		   Test what types of keys are supported.

		   Normally you would just test if the capability is present, however
		   for PKey, you also need to test which types of keys are available. So
		   if you want to figure out if RSA keys are supported, you need to 
		   do something like:
		   \code
		   if(!QCA::isSupported("pkey") ||
		      !QCA::PKey::supportedTypes().contains(QCA::PKey::RSA)) {
		      // then there is no RSA key support
		   } else {
		      // there is RSA key support
		   }
		   \endcode

		   To make things a bit more complex, supportedTypes() only
		   checks for basic functionality. If you want to check that 
		   you can do operations with PEM or DER (eg toPEM(), fromPEM(), and
		   the equivalent DER and PEMfile operations, plus anything else
		   that uses them, including the constructor form that takes a fileName), 
		   then you need to check for supportedIOTypes() instead.

		   \sa supportedIOTypes()
		*/
		static QList<Type> supportedTypes(const QString &provider = QString());

		/**
		   Test what types of keys are supported for IO operations

		   If you are using PKey DER or PEM operations, then you need
		   to check for appropriate support using this method. For example,
		   if you want to check if you can export or import an RSA key, then
		   you need to do something like:
		   \code
		   if(!QCA::isSupported("pkey") ||
		      !QCA::PKey::supportedIOTypes().contains(QCA::PKey::RSA)) {
		      // then there is no RSA key IO support
		   } else {
		      // there is RSA key IO support
		   }
		   \endcode

		   Note that if you only want to check for basic functionality
		   (ie not PEM or DER import/export), then you can use supportedTypes().
		   There is no need to use both - if the key type is supported for 
		   IO, then is also supported for basic operations.

		   \sa supportedTypes()
		*/
		static QList<Type> supportedIOTypes(const QString &provider = QString());

		/**
		   Test if the key is null (empty)

		   \return true if the key is null
		*/
		bool isNull() const;

		/**
		   Report the Type of key (eg RSA, DSA or Diffie Hellman)
		   
		   \sa isRSA, isDSA and isDH for boolean tests.
		*/
		Type type() const;

		/**
		   Report the number of bits in the key
		*/
		int bitSize() const;

		/**
		   Test if the key is an RSA key
		*/
		bool isRSA() const;

		/**
		   Test if the key is a DSA key
		*/
		bool isDSA() const;

		/**
		   Test if the key is a Diffie Hellman key
		*/
		bool isDH() const;

		/**
		   Test if the key is a public key
		*/
		bool isPublic() const;	

		/**
		   Test if the key is a private key
		*/
		bool isPrivate() const;

		/**
		   Test if the key data can be exported.  If the key resides on a smart
		   card or other such device, this will likely return false.
		*/
		bool canExport() const;

		/**
		   Test if the key can be used for key agreement
		*/
		bool canKeyAgree() const;

		/**
		   Interpret this key as a PublicKey

		   \sa toRSAPublicKey(), toDSAPublicKey() and toDHPublicKey()
		   for protected forms of this call.
		*/
		PublicKey toPublicKey() const;

		/**
		   Interpret this key as a PrivateKey
		*/
		PrivateKey toPrivateKey() const;

		/**
		   test if two keys are equal
		*/
		bool operator==(const PKey &a) const;

		/**
		   test if two keys are not equal
		*/
		bool operator!=(const PKey &a) const;

	protected:
		/**
		   Create a key of the specified type
		*/
		PKey(const QString &type, const QString &provider);

		/**
		   Set the key
		*/
		void set(const PKey &k);

		/**
		   Interpret this key as an RSAPublicKey

		   \note This function is essentially a convenience cast - if the
		   key was created as a DSA key, this function cannot turn it into 
		   an RSA key.

		   \sa toPublicKey() for the public version of this method
		*/
		RSAPublicKey toRSAPublicKey() const;

		/**
		   Interpret this key as an  RSAPrivateKey

		   \note This function is essentially a convenience cast - if the
		   key was created as a DSA key, this function cannot turn it into 
		   a RSA key.

		   \sa toPrivateKey() for the public version of this method
		*/
		RSAPrivateKey toRSAPrivateKey() const;

		/**
		   Interpret this key as an DSAPublicKey

		   \note This function is essentially a convenience cast - if the
		   key was created as an RSA key, this function cannot turn it into 
		   a DSA key.

		   \sa toPublicKey() for the public version of this method
		*/
		DSAPublicKey toDSAPublicKey() const;

		/**
		   Interpret this key as a DSAPrivateKey

		   \note This function is essentially a convenience cast - if the
		   key was created as an RSA key, this function cannot turn it into 
		   a DSA key.

		   \sa toPrivateKey() for the public version of this method
		*/
		DSAPrivateKey toDSAPrivateKey() const;

		/**
		   Interpret this key as an DHPublicKey

		   \note This function is essentially a convenience cast - if the
		   key was created as a DSA key, this function cannot turn it into 
		   a DH key.

		   \sa toPublicKey() for the public version of this method
		*/
		DHPublicKey toDHPublicKey() const;

		/**
		   Interpret this key as a DHPrivateKey

		   \note This function is essentially a convenience cast - if the
		   key was created as a DSA key, this function cannot turn it into 
		   a DH key.

		   \sa toPrivateKey() for the public version of this method
		*/
		DHPrivateKey toDHPrivateKey() const;

	private:
		void assignToPublic(PKey *dest) const;
		void assignToPrivate(PKey *dest) const;

		class Private;
		Private *d;
	};

	/**
	   \class PublicKey qca_publickey.h QtCrypto

	   Generic public key
	*/
	class QCA_EXPORT PublicKey : public PKey
	{
	public:
		/**
		   Create an empty (null) public key
		*/
		PublicKey();

		/**
		   Create a public key based on a specified private key

		   \param k the private key to extract the public key parts from
		*/
		PublicKey(const PrivateKey &k);

		/**
		   Import a public key from a PEM representation in a file

		   \param fileName the name of the file containing the public key

		   \sa fromPEMFile for an alternative method
		*/
		PublicKey(const QString &fileName);

		/**
		   Convenience method to convert this key to an RSAPublicKey

		   Note that if the key is not an RSA key (eg it is DSA or DH),
		   then this will produce a null key.
		*/
		RSAPublicKey toRSA() const;

		/**
		   Convenience method to convert this key to a DSAPublicKey

		   Note that if the key is not an DSA key (eg it is RSA or DH),
		   then this will produce a null key.
		*/
		DSAPublicKey toDSA() const;

		/**
		   Convenience method to convert this key to a DHPublicKey

		   Note that if the key is not an DH key (eg it is DSA or RSA),
		   then this will produce a null key.
		*/
		DHPublicKey toDH() const;

		/**
		   Test if this key can be used for encryption

		   \return true if the key can be used for encryption
		*/
		bool canEncrypt() const;

		/**
		   Test if the key can be used for verifying signatures

		   \return true of the key can be used for verification
		*/
		bool canVerify() const;

		// encrypt / verify
		/**
		   The maximum message size that can be encrypted with a specified algorithm

		   \param alg the algorithm to check
		*/
		int maximumEncryptSize(EncryptionAlgorithm alg) const;

		/**
		   Encrypt a message using a specified algorithm

		   \param a the message to encrypt
		   \param alg the algorithm to use
		*/
		QSecureArray encrypt(const QSecureArray &a, EncryptionAlgorithm alg);

		/**
		   Initialise the signature verification process

		   \param alg the algorithm to use for signing
		   \param format the specific format to use, for DSA
		*/
		void startVerify(SignatureAlgorithm alg, SignatureFormat format = DefaultFormat);

		/**
		   Update the signature verification process with more data

		   \param a the array containing the data that should be added to the signature
		*/
		void update(const QSecureArray &a);

		/**
		   Check the signature is valid for the message

		   The process to check that a signature is correct is shown below:
		   \code
		   // note that pubkey is a PublicKey
		   if( pubkey.canVerify() ) {
		       pubkey.startVerify( QCA::EMSA3_MD5 );
		       pubkey.update( theMessage ); // might be called multiple times
	               if ( pubkey.validSignature( theSignature ) ) {
		          // then signature is valid
		       } else {
			   // then signature is invalid
		       }
                   }		   
                   \endcode
		   
		   \param sig the signature to check 

		   \return true if the signature is correct
		*/
		bool validSignature(const QSecureArray &sig);

		/**
		   Single step message verification

		   If you have the whole message to be verified, then this offers a more
		   convenient approach to verification.

		   \param a the message to check the signature on
		   \param sig the signature to be checked
		   \param alg the algorithm to use
		   \param format the signature format to use, for DSA
		   
		   \return true if the signature is valid for the message
		*/
		bool verifyMessage(const QSecureArray &a, const QSecureArray &sig, SignatureAlgorithm alg, SignatureFormat format = DefaultFormat);


		/**
		   Export the key in Distinguished Encoding Rules (DER) format
		*/
		QSecureArray toDER() const;

		/**
		   Export the key in Privacy Enhanced Mail (PEM) format

		   \sa toPEMFile provides a convenient way to save the PEM encoded key to a file
		   \sa fromPEM provides an inverse of toPEM, converting the PEM encoded key back to a PublicKey
		*/
		QString toPEM() const;

		/**
		   Export the key in Privacy Enhanced Mail (PEM) to a file

		   \param fileName the name (and path, if necessary) of the file to save the
		   PEM encoded key to.

		   \sa toPEM for a version that exports to a QString, which may be useful if you
		   need to do more sophisticated handling
		   \sa fromPEMFile provides an inverse of toPEMFile, reading a PEM encoded key from a file
		*/
		bool toPEMFile(const QString &fileName) const;

		/**
		   Import a key in Distinguished Encoding Rules (DER) format

		   This function takes a binary array, which is assumed to contain a public key
		   in DER encoding, and returns the key. Unless you don't care whether the import
		   succeeded, you should test the result, as shown below.

		   \code
		   QCA::ConvertResult conversionResult;
		   QCA::PublicKey publicKey = QCA::PublicKey::fromDER(keyArray, &conversionResult);
		   if (! QCA::ConvertGood == conversionResult) {
		       std::cout << "Public key read failed" << std::endl;
		   }
		   \endcode

		   \param a the array containing a DER encoded key
		   \param result pointer to a variable, which returns whether the conversion succeeded (ConvertGood) or not
		   \param provider the name of the provider to use for the import.
		*/
		static PublicKey fromDER(const QSecureArray &a, ConvertResult *result = 0, const QString &provider = QString());

		/**
		   Import a key in Privacy Enhanced Mail (PEM) format

		   This function takes a string, which is assumed to contain a public key
		   in PEM encoding, and returns that key. Unless you don't care whether the import
		   succeeded, you should test the result, as shown below.

		   \code
		   QCA::ConvertResult conversionResult;
		   QCA::PublicKey publicKey = QCA::PublicKey::fromPEM(keyAsString, &conversionResult);
		   if (! QCA::ConvertGood == conversionResult) {
		       std::cout << "Public key read failed" << std::endl;
		   }
		   \endcode

		   \param s the string containing a PEM encoded key
		   \param result pointer to a variable, which returns whether the conversion succeeded (ConvertGood) or not
		   \param provider the name of the provider to use for the import.

		   \sa toPEM, which provides an inverse of fromPEM()
		   \sa fromPEMFile, which provides an import direct from a file.
		*/
		static PublicKey fromPEM(const QString &s, ConvertResult *result = 0, const QString &provider = QString());

		/**
		   Import a key in Privacy Enhanced Mail (PEM) format from a file

		   This function takes the name of a file, which is assumed to contain a public key
		   in PEM encoding, and returns that key. Unless you don't care whether the import
		   succeeded, you should test the result, as shown below.

		   \code
		   QCA::ConvertResult conversionResult;
		   QCA::PublicKey publicKey = QCA::PublicKey::fromPEMFile(fileName, &conversionResult);
		   if (! QCA::ConvertGood == conversionResult) {
		       std::cout << "Public key read failed" << std::endl;
		   }
		   \endcode

		   \param fileName a string containing the name of the file
		   \param result pointer to a variable, which returns whether the conversion succeeded (ConvertGood) or not
		   \param provider the name of the provider to use for the import.

		   \sa toPEMFile, which provides an inverse of fromPEMFile()
		   \sa fromPEM, which provides an import from a string

		   \note there is also a constructor form that can import from a file
		*/
		static PublicKey fromPEMFile(const QString &fileName, ConvertResult *result = 0, const QString &provider = QString());

	protected:
		/**
		   Create a new key of a specified type

		   \param type the type of key to create
		   \param provider the provider to use, if required
		*/
		PublicKey(const QString &type, const QString &provider);
	};

	/**
	   \class PrivateKey qca_publickey.h QtCrypto

	   Generic private key
	*/
	class QCA_EXPORT PrivateKey : public PKey
	{
	public:
		/**
		   Create an empty private key
		*/
		PrivateKey();

		/**
		   Import a private key from a PEM representation in a file

		   \param fileName the name of the file containing the private key
		   \param passphrase the pass phrase for the private key

		   \sa fromPEMFile for an alternative method
		*/
		PrivateKey(const QString &fileName, const QSecureArray &passphrase = QSecureArray());

		/**
		   Interpret / convert the key to an RSA key
		*/
		RSAPrivateKey toRSA() const;
		
		/**
		   Interpret / convert the key to a DSA key
		*/
		DSAPrivateKey toDSA() const;

		/**
		   Interpret / convert the key to a Diffie-Hellman key
		*/
		DHPrivateKey toDH() const;

		/**
		   Test if this key can be used for decryption

		   \return true if the key can be used for decryption
		*/
		bool canDecrypt() const;

		/**
		   Test if this key can be used for signing

		   \return true if the key can be used to make a signature
		*/
		bool canSign() const;

		// decrypt / sign / key agreement
		/**
		   Decrypt the message
		   
		   \param in the cipher (encrypted) data
		   \param out the plain text data
		   \param alg the algorithm to use
		*/
		bool decrypt(const QSecureArray &in, QSecureArray *out, EncryptionAlgorithm alg);

		/**
		   Initialise the message signature process

		   \param alg the algorithm to use for the message signature process
		   \param format the signature format to use, for DSA
		*/
		void startSign(SignatureAlgorithm alg, SignatureFormat format = DefaultFormat);

		/**
		   Update the signature process

		   \param a the message to use to update the signature
		*/
		void update(const QSecureArray &a);

		/**
		   The resulting signature
		*/
		QSecureArray signature();

		/**
		   One step signature process

		   \param a the message to sign
		   \param alg the algorithm to use for the signature
		   \param format the signature format to use, for DSA

		   \return the signature
		*/
		QSecureArray signMessage(const QSecureArray &a, SignatureAlgorithm alg, SignatureFormat format = DefaultFormat);

		/**
		   Derive a shared secret key from a public key

		   \param theirs the public key to derive from
		*/
		SymmetricKey deriveKey(const PublicKey &theirs);

		// import / export
		/**
		   List the supported Password Based Encryption Algorithms that can be used
		   to protect the key.

		   \param provider the provider to use, if a particular provider is required
		*/
		static QList<PBEAlgorithm> supportedPBEAlgorithms(const QString &provider = QString());

		/**
		   Export the key in Distinguished Encoding Rules (DER) format

		   \param passphrase the pass phrase to use to protect the key
		   \param pbe the symmetric encryption algorithm to use to protect the key

		   \sa fromDER provides an inverse of toDER, converting the DER encoded key back to a PrivateKey
		*/
		QSecureArray toDER(const QSecureArray &passphrase = QSecureArray(), PBEAlgorithm pbe = PBEDefault) const;

		/**
		   Export the key in Privacy Enhanced Mail (PEM) format

		   \param passphrase the pass phrase to use to protect the key
		   \param pbe the symmetric encryption algorithm to use to protect the key

		   \sa toPEMFile provides a convenient way to save the PEM encoded key to a file
		   \sa fromPEM provides an inverse of toPEM, converting the PEM encoded key back to a PrivateKey
		*/
		QString toPEM(const QSecureArray &passphrase = QSecureArray(), PBEAlgorithm pbe = PBEDefault) const;

		/**
		   Export the key in Privacy Enhanced Mail (PEM) format to a file

		   \param fileName the name (and path, if required) that the key should be exported to.
		   \param passphrase the pass phrase to use to protect the key
		   \param pbe the symmetric encryption algorithm to use to protect the key

		   \return true if the export succeeds

		   \sa toPEM provides a convenient way to save the PEM encoded key to a file
		   \sa fromPEM provides an inverse of toPEM, converting the PEM encoded key back to a PrivateKey
		*/
		bool toPEMFile(const QString &fileName, const QSecureArray &passphrase = QSecureArray(), PBEAlgorithm pbe = PBEDefault) const;

		/**
		   Import the key from Distinguished Encoding Rules (DER) format

		   \param a the array containing the DER representation of the key
		   \param passphrase the pass phrase that is used to protect the key
		   \param result a pointer to a ConvertResult, that if specified, will be set to reflect the result
		   of the import
		   \param provider the provider to use, if a particular provider is required

		   \sa toDER provides an inverse of fromDER, exporting the key to an array
		*/ 
		static PrivateKey fromDER(const QSecureArray &a, const QSecureArray &passphrase = QSecureArray(), ConvertResult *result = 0, const QString &provider = QString());

		/**
		   Import the key from Privacy Enhanced Mail (PEM) format

		   \param s the string containing the PEM representation of the key
		   \param passphrase the pass phrase that is used to protect the key
		   \param result a pointer to a ConvertResult, that if specified, will be set to reflect the result
		   of the import
		   \param provider the provider to use, if a particular provider is required

		   \sa toPEM provides an inverse of fromPEM, exporting the key to a string in PEM encoding.
		*/ 
		static PrivateKey fromPEM(const QString &s, const QSecureArray &passphrase = QSecureArray(), ConvertResult *result = 0, const QString &provider = QString());

		/**
		   Import the key in Privacy Enhanced Mail (PEM) format from a file

		   \param fileName the name (and path, if required) of the file containing the PEM representation of the key
		   \param passphrase the pass phrase that is used to protect the key
		   \param result a pointer to a ConvertResult, that if specified, will be set to reflect the result
		   of the import
		   \param provider the provider to use, if a particular provider is required

		   \sa toPEMFile provides an inverse of fromPEMFile
		   \sa fromPEM which allows import from a string

		   \note there is also a constructor form, that allows you to create the key directly
		*/ 
		static PrivateKey fromPEMFile(const QString &fileName, const QSecureArray &passphrase = QSecureArray(), ConvertResult *result = 0, const QString &provider = QString());

	protected:
		/**
		   Create a new private key

		   \param type the type of key to create
		   \param provider the provider to use, if a specific provider is required.
		*/
		PrivateKey(const QString &type, const QString &provider);
	};

	/**
	   \class KeyGenerator qca_publickey.h QtCrypto

	   Class for generating asymmetric key pairs

	   This class is used for generating asymmetric keys (public/private key pairs)
	*/
	class QCA_EXPORT KeyGenerator : public QObject
	{
		Q_OBJECT
	public:
		/**
		   Create a new key generator

		   \param parent the parent object, if applicable
		*/
		KeyGenerator(QObject *parent = 0);

		~KeyGenerator();

		/**
		   Test whether the key generator is set to operate in blocking mode, or not

		   \return true if the key generator is in blocking mode

		   \sa setBlocking
		*/
		bool blocking() const;

		/**
		   Set whether the key generator is in blocking mode, nor not

		   \param b if true, the key generator will be set to operate in blocking mode,
		   otherwise it will operate in non-blocking mode

		   \sa blocking()
		*/
		void setBlocking(bool b);

		/**
		   Test if the key generator is currently busy, or not

		   \return true if the key generator is busy generating a key already
		*/
		bool isBusy() const;

		/**
		   Generate an RSA key of the specified length

		   This method creates both the public key and corresponding private key. You
		   almost certainly want to extract the public key part out - see PKey::toPublicKey
		   for an easy way.

		   Key length is a tricky judgment - using less than 2048 is probably being
		   too liberal for long term use. Don't use less than 1024 without serious
		   analysis.
		   
		   \param bits the length of key that is required
		   \param exp the exponent - typically 3, 17 or 65537
		   \param provider the name of the provider to use, if a particular provider is required
		*/
		PrivateKey createRSA(int bits, int exp = 65537, const QString &provider = QString());

		/**
		   Generate a DSA key

		   This method creates both the public key and corresponding private key. You
		   almost certainly want to extract the public key part out - see PKey::toPublicKey
		   for an easy way.

		   \param domain the discrete logarithm group that this key should be generated from
		   \param provider the name of the provider to use, if a particular provider is required

		   \note Not every DLGroup makes sense for DSA. You should use one of DSA_512,
		   DSA_768 and DSA_1024.
		*/
		PrivateKey createDSA(const DLGroup &domain, const QString &provider = QString());

		/**
		   Generate a Diffie-Hellman key

		   This method creates both the public key and corresponding private key. You
		   almost certainly want to extract the public key part out - see PKey::toPublicKey
		   for an easy way.

		   \param domain the discrete logarithm group that this key should be generated from
		   \param provider the name of the provider to use, if a particular provider is required
		   \note For compatibility, you should use one of the IETF_ groupsets as the
		   domain argument.
		*/
		PrivateKey createDH(const DLGroup &domain, const QString &provider = QString());

		/**
		   Return the last generated key

		   This is really only useful when you are working with non-blocking key generation
		*/
		PrivateKey key() const;

		/**
		   Create a new discrete logarithm group

		   \param set the set of discrete logarithm parameters to generate from
		   \param provider the name of the provider to use, if a particular provider is required.
		*/
		DLGroup createDLGroup(QCA::DLGroupSet set, const QString &provider = QString());

		/**
		   The current discrete logarithm group 
		*/
		DLGroup dlGroup() const;

	signals:
		/**
		   Emitted when the key generation is complete.

		   This is only used in non-blocking mode
		*/
		void finished();

	private:
		class Private;
		friend class Private;
		Private *d;
	};

	/**
	   \class RSAPublicKey qca_publickey.h QtCrypto

	   RSA Public Key
	*/
	class QCA_EXPORT RSAPublicKey : public PublicKey
	{
	public:
		/**
		   Generate an empty RSA public key
		*/
		RSAPublicKey();

		/**
		   Generate an RSA public key from specified parameters

		   \param n the public key value
		   \param e the public key exponent
		   \param provider the provider to use, if a particular provider is required
		*/
		RSAPublicKey(const QBigInteger &n, const QBigInteger &e, const QString &provider = QString());

		/**
		   Extract the public key components from an RSA private key

		   \param k the private key to use as the basis for the public key
		*/
		RSAPublicKey(const RSAPrivateKey &k);

		/**
		   The public key value

		   This value is the actual public key value (the product of p and q, the random prime numbers
		   used to generate the RSA key), also known as the public modulus.
		*/
		QBigInteger n() const;

		/**
		   The public key exponent

		   This value is the exponent chosen in the original key generator step
		*/
		QBigInteger e() const;
	};

	/**
	   \class RSAPrivateKey qca_publickey.h QtCrypto

	   RSA Private Key
	*/
	class QCA_EXPORT RSAPrivateKey : public PrivateKey
	{
	public:
		/**
		   Generate an empty RSA private key
		*/
		RSAPrivateKey();

		/**
		   Generate an RSA private key from specified parameters

		   \param n the public key value
		   \param e the public key exponent
		   \param p one of the two chosen primes
		   \param q the other of the two chosen primes
		   \param d inverse of the exponent, modulo (p-1)(q-1)
		   \param provider the provider to use, if a particular provider is required
		*/
		RSAPrivateKey(const QBigInteger &n, const QBigInteger &e, const QBigInteger &p, const QBigInteger &q, const QBigInteger &d, const QString &provider = QString());

		/**
		   The public key value

		   This value is the actual public key value (the product of p and q, the random prime numbers
		   used to generate the RSA key), also known as the public modulus.
		*/
		QBigInteger n() const;

		/**
		   The public key exponent

		   This value is the exponent chosen in the original key generator step
		*/
		QBigInteger e() const;

		/**
		   One of the two random primes used to generate the private key
		*/
		QBigInteger p() const;

		/**
		   The second of the two random primes used to generate the private key
		*/
		QBigInteger q() const;

		/**
		   The inverse of the exponent, module (p-1)(q-1)
		*/
		QBigInteger d() const;
	};

	/**
	   \class DSAPublicKey qca_publickey.h QtCrypto

	   Digital Signature %Algorithm Public Key
	*/
	class QCA_EXPORT DSAPublicKey : public PublicKey
	{
	public:
		/**
		   Create an empty DSA public key
		*/
		DSAPublicKey();

		/**
		   Create a DSA public key

		   \param domain the discrete logarithm group to use
		   \param y the public random value
		   \param provider the provider to use, if a specific provider is required
		*/
		DSAPublicKey(const DLGroup &domain, const QBigInteger &y, const QString &provider = QString());

		/**
		   Create a DSA public key from a specified private key

		   \param k the DSA private key to use as the source
		*/
		DSAPublicKey(const DSAPrivateKey &k);

		/**
		   The discrete logarithm group that is being used
		*/
		DLGroup domain() const;

		/**
		   The public random value associated with this key
		*/
		QBigInteger y() const;
	};

	/**
	   \class DSAPrivateKey qca_publickey.h QtCrypto

	   Digital Signature %Algorithm Private Key
	*/
	class QCA_EXPORT DSAPrivateKey : public PrivateKey
	{
	public:
		/**
		   Create an empty DSA private key
		*/
		DSAPrivateKey();

		/**
		   Create a DSA public key

		   \param domain the discrete logarithm group to use
		   \param y the public random value
		   \param x the private random value
		   \param provider the provider to use, if a specific provider is required
		*/
		DSAPrivateKey(const DLGroup &domain, const QBigInteger &y, const QBigInteger &x, const QString &provider = QString());

		/**
		   The discrete logarithm group that is being used
		*/
		DLGroup domain() const;

		/**
		   the public random value
		*/
		QBigInteger y() const;

		/**
		   the private random value
		*/
		QBigInteger x() const;
	};

	/**
	   \class DHPublicKey qca_publickey.h QtCrypto

	   Diffie-Hellman Public Key
	*/
	class QCA_EXPORT DHPublicKey : public PublicKey
	{
	public:
		/** 
		    Create an empty Diffie-Hellman public key
		*/
		DHPublicKey();

		/**
		   Create a Diffie-Hellman public key

		   \param domain the discrete logarithm group to use
		   \param y the public random value
		   \param provider the provider to use, if a specific provider is required
		*/
		DHPublicKey(const DLGroup &domain, const QBigInteger &y, const QString &provider = QString());

		/**
		   Create a Diffie-Hellman public key from a specified private key

		   \param k the Diffie-Hellman private key to use as the source
		*/
		DHPublicKey(const DHPrivateKey &k);

		/**
		   The discrete logarithm group that is being used
		*/
		DLGroup domain() const;

		/**
		   The public random value associated with this key
		*/
		QBigInteger y() const;
	};

	/**
	   \class DHPrivateKey qca_publickey.h QtCrypto

	   Diffie-Hellman Private Key
	*/
	class QCA_EXPORT DHPrivateKey : public PrivateKey
	{
	public:
		/**
		   Create an empty Diffie-Hellman private key
		*/
		DHPrivateKey();

		/**
		   Create a Diffie-Hellman privat key

		   \param domain the discrete logarithm group to use
		   \param y the public random value
		   \param x the private random value
		   \param provider the provider to use, if a particular provider is required
		*/
		DHPrivateKey(const DLGroup &domain, const QBigInteger &y, const QBigInteger &x, const QString &provider = QString());

		/**
		   The discrete logarithm group that is being used
		*/
		DLGroup domain() const;

		/**
		   The public random value associated with this key
		*/
		QBigInteger y() const;

		/**
		   The private random value associated with this key
		*/
		QBigInteger x() const;
	};
}

#endif
