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

#include "qca_keystore.h"

#include "qcaprovider.h"

namespace QCA {

Provider::Context *getContext(const QString &type, Provider *p);

/*
  How this stuff works:

  KeyStoreListContext is queried for a list of KeyStoreContexts.  A signal
  is used to indicate when the list may have changed, so polling for changes
  is not necessary.  Every context object created internally by the provider
  will have a unique contextId, and this is used for detecting changes.  Even
  if a user removes and inserts the same smart card device, which has the
  same deviceId, the contextId will ALWAYS be different.  If a previously
  known contextId is missing from a later queried list, then it means the
  associated KeyStoreContext has been deleted by the provider (the manager
  here does not delete them, it just throws away any references).  It is
  recommended that the provider just use a counter for the contextId,
  incrementing the value anytime a new context is made.
*/

/*
- all keystorelistcontexts in a side thread
- all keystores also exist in the same side thread
- mutex protect the entire system, so that nothing happens simultaneously:
  - every keystore / keystoreentry access method
  - provider scanning
  - keystore removal
- call keystoremanager.start() to initiate system
- waitfordiscovery counts for all providers found on first scan

- maybe keystore should thread-safe protect all calls

scenarios to handle:
  - ksm.start shouldn't block
  - keystore in available list, but gone by the time it is requested
  - keystore is unavailable during a call to a keystoreentry method
  - keystore/keystoreentry methods called simultaneously from different threads
  - and of course, objects from keystores should work, despite being created
    in the keystore thread
*/

//----------------------------------------------------------------------------
// KeyStoreTracker
//----------------------------------------------------------------------------

static int tracker_id_at = 0;

class KeyStoreTracker : public QObject
{
	Q_OBJECT
public:
	static KeyStoreTracker *self;

	class Item
	{
	public:
		// combine keystore owner and contextid into a single id
		int trackerId;

		// number of times the keystore has been updated
		int updateCount;

		// keystore context
		KeyStoreListContext *owner;
		int storeContextId;

		// properties
		QString storeId;
		QString name;
		KeyStore::Type type;
		bool isReadOnly;

		Item() : trackerId(-1), updateCount(0), owner(0), storeContextId(-1)
		{
		}
	};

	QSet<Provider*> providerSet;
	QList<KeyStoreListContext*> sources;
	QList<Item> items;
	QHash<KeyStoreListContext*,bool> busyStatus;
	QMutex m;
	bool startedAll;
	bool busy;

	KeyStoreTracker()
	{
		self = this;

		qRegisterMetaType<QCA::KeyStoreEntry>("QCA::KeyStoreEntry");
		qRegisterMetaType< QList<QCA::KeyStoreEntry> >("QList<QCA::KeyStoreEntry>");
		qRegisterMetaType< QList<QCA::KeyStoreEntry::Type> >("QList<QCA::KeyStoreEntry::Type>");

		connect(this, SIGNAL(updated_p()), SIGNAL(updated()), Qt::QueuedConnection);

		startedAll = false;
		busy = true; // we start out busy
	}

	~KeyStoreTracker()
	{
		qDeleteAll(sources);
		self = 0;
	}

	static KeyStoreTracker *instance()
	{
		return self;
	}

	// thread-safe
	bool isBusy()
	{
		// TODO
		QMutexLocker locker(&m);
		return busy;
	}

	// thread-safe
	QList<KeyStoreTracker::Item> getItems()
	{
		// TODO
		QMutexLocker locker(&m);
		return items;
	}

	// thread-safe
	QString getDText()
	{
		// TODO
		return QString();
	}

	// thread-safe
	void clearDText()
	{
		// TODO
	}

public slots:
	void start()
	{
		// grab providers (and default)
		//scanForPlugins();
		ProviderList list = providers();
		list.append(defaultProvider());

		for(int n = 0; n < list.count(); ++n)
		{
			Provider *p = list[n];
			if(p->features().contains("keystorelist") && !providerSet.contains(p))
				startProvider(p);
		}

		startedAll = true;
	}

	void start(const QString &provider)
	{
		// grab providers (and default)
		ProviderList list = providers();
		list.append(defaultProvider());

		Provider *p = 0;
		for(int n = 0; n < list.count(); ++n)
		{
			if(list[n]->name() == provider)
			{
				p = list[n];
				break;
			}
		}

		if(p)
			startProvider(p);
	}

	void scan()
	{
		if(startedAll)
			start();
	}

	QList<QCA::KeyStoreEntry> entryList(int trackerId)
	{
		// TODO
		QList<KeyStoreEntry> out;
		int contextId = -1;
		KeyStoreListContext *c;
		m.lock();
		foreach(Item i, items)
		{
			if(i.trackerId == trackerId)
			{
				contextId = i.storeContextId;
				c = i.owner;
				break;
			}
		}
		m.unlock();
		if(contextId == -1)
			return out;
		QList<KeyStoreEntryContext*> list = c->entryList(contextId);
		for(int n = 0; n < list.count(); ++n)
		{
			KeyStoreEntry entry;
			entry.change(list[n]);
			setEntryAvailable(&entry);
			out.append(entry);
		}
		return out;
	}

	QList<QCA::KeyStoreEntry::Type> entryTypes(int trackerId)
	{
		// TODO
		QList<KeyStoreEntry::Type> out;
		int contextId = -1;
		KeyStoreListContext *c;
		m.lock();
		foreach(Item i, items)
		{
			if(i.trackerId == trackerId)
			{
				contextId = i.storeContextId;
				c = i.owner;
				break;
			}
		}
		m.unlock();
		if(contextId == -1)
			return out;
		return c->entryTypes(contextId);
	}

	// hack with void *
	void *entry(const QString &storeId, const QString &entryId)
	{
		KeyStoreListContext *c = 0;
		int contextId;
		m.lock();
		foreach(Item i, items)
		{
			if(i.storeId == storeId)
			{
				c = i.owner;
				contextId = i.storeContextId;
				break;
			}
		}
		m.unlock();
		if(!c)
			return 0;

		return c->entry(contextId, entryId);
	}

	// hack with void *
	void *entryPassive(const QString &entryId)
	{
		foreach(Item i, items)
		{
			// "is this yours?"
			KeyStoreEntryContext *e = i.owner->entryPassive(i.storeId, entryId);
			if(e)
				return e;
		}
		return 0;
	}

signals:
	// emit this when items or busy state changes
	void updated();
	void updated_p();

private:
	void startProvider(Provider *p)
	{
		KeyStoreListContext *c = static_cast<KeyStoreListContext *>(getContext("keystorelist", p));
		if(!c)
			return;
		providerSet += p;
		sources += c;
		busyStatus[c] = true;
		connect(c, SIGNAL(busyStart()), SLOT(ksl_busyStart()));
		connect(c, SIGNAL(busyEnd()), SLOT(ksl_busyEnd()));
		connect(c, SIGNAL(updated()), SLOT(ksl_updated()));
		connect(c, SIGNAL(diagnosticText(const QString &)), SLOT(ksl_diagnosticText(const QString &)));
		connect(c, SIGNAL(storeUpdated(int)), SLOT(ksl_storeUpdated(int)));
		c->start();
		c->setUpdatesEnabled(true);
		//printf("started provider: %s\n", qPrintable(p->name()));
	}

	bool updateStores(KeyStoreListContext *c)
	{
		QList<int> keyStores = c->keyStores();
		foreach(int id, keyStores)
		{
			QMutexLocker locker(&m);

			// FIXME: this code is just enough to get qcatool
			//   operational, but is mostly incomplete/wrong
			//   in any other case

			// don't add dups
			bool found = false;
			QString storeId = c->storeId(id);
			foreach(Item i, items)
			{
				if(i.storeId == storeId)
				{
					found = true;
					break;
				}
			}
			if(found)
				continue;

			Item i;
			i.trackerId = tracker_id_at++;
			i.updateCount = 0;
			i.owner = c;
			i.storeContextId = id;
			i.storeId = c->storeId(id);
			i.name = c->name(id);
			i.type = c->type(id);
			i.isReadOnly = c->isReadOnly(id);
			items += i;
		}
		return true;
	}

	// implemented below, after KeyStoreEntry
	void setEntryAvailable(KeyStoreEntry *e);

private slots:
	void ksl_busyStart()
	{
		KeyStoreListContext *c = (KeyStoreListContext *)sender();
		//printf("busyStart: [%s]\n", qPrintable(c->provider()->name()));
		busyStatus[c] = true;
	}

	void ksl_busyEnd()
	{
		KeyStoreListContext *c = (KeyStoreListContext *)sender();
		//printf("busyEnd: [%s]\n", qPrintable(c->provider()->name()));
		bool changed = updateStores(c);
		busyStatus[c] = false;
		bool any_busy = false;
		QHashIterator<KeyStoreListContext*,bool> it(busyStatus);
		while(it.hasNext())
		{
			it.next();
			if(it.value())
			{
				any_busy = true;
				break;
			}
		}

		if(!any_busy)
		{
			m.lock();
			busy = false;
			m.unlock();
		}

		if(!any_busy || changed)
		{
			//printf("emitting update\n");
			emit updated_p();
		}
	}

	void ksl_updated()
	{
		// TODO
	}

	void ksl_diagnosticText(const QString &str)
	{
		// TODO
		Q_UNUSED(str);
	}

	void ksl_storeUpdated(int id)
	{
		// TODO
		Q_UNUSED(id);
	}
};

KeyStoreTracker *KeyStoreTracker::self = 0;

//----------------------------------------------------------------------------
// KeyStoreThread
//----------------------------------------------------------------------------
class KeyStoreThread : public SyncThread
{
	Q_OBJECT
public:
	KeyStoreTracker *tracker;

	KeyStoreThread(QObject *parent = 0) : SyncThread(parent)
	{
	}

	~KeyStoreThread()
	{
		stop();
	}

	void atStart()
	{
		tracker = new KeyStoreTracker;
	}

	void atEnd()
	{
		delete tracker;
	}
};

//----------------------------------------------------------------------------
// KeyStoreGlobal
//----------------------------------------------------------------------------
class KeyStoreManagerGlobal;

Q_GLOBAL_STATIC(QMutex, ksm_mutex)
static KeyStoreManagerGlobal *g_ksm = 0;

class KeyStoreManagerGlobal
{
public:
	KeyStoreThread *thread;

	KeyStoreManagerGlobal()
	{
		thread = new KeyStoreThread;
		thread->moveToThread(QCoreApplication::instance()->thread());
		thread->start();
	}

	~KeyStoreManagerGlobal()
	{
		delete thread;
	}
};

static QVariant trackercall(const char *method, const QVariantList &args = QVariantList())
{
	QVariant ret;
	bool ok;
	ret = g_ksm->thread->call(KeyStoreTracker::instance(), method, args, &ok);
	Q_ASSERT(ok);
	return ret;
}

//----------------------------------------------------------------------------
// KeyStoreEntry
//----------------------------------------------------------------------------
class KeyStoreEntry::Private
{
public:
	bool available;
	bool accessible;

	Private()
	{
		available = false;
		accessible = false;
	}
};

KeyStoreEntry::KeyStoreEntry()
:d(new Private)
{
}

KeyStoreEntry::KeyStoreEntry(const QString &id)
:d(new Private)
{
	KeyStoreEntryContext *c = (KeyStoreEntryContext *)qVariantValue<void*>(trackercall("entryPassive", QVariantList() << id));
	if(c)
		change(c);
}

KeyStoreEntry::KeyStoreEntry(const KeyStoreEntry &from)
:Algorithm(from), d(new Private(*from.d))
{
}

KeyStoreEntry::~KeyStoreEntry()
{
	delete d;
}

KeyStoreEntry & KeyStoreEntry::operator=(const KeyStoreEntry &from)
{
	Algorithm::operator=(from);
	*d = *from.d;
	return *this;
}

bool KeyStoreEntry::isNull() const
{
	return (!context() ? true : false);
}

bool KeyStoreEntry::isAvailable() const
{
	return d->available;
}

bool KeyStoreEntry::isAccessible() const
{
	return d->accessible;
}

KeyStoreEntry::Type KeyStoreEntry::type() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->type();
}

QString KeyStoreEntry::name() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->name();
}

QString KeyStoreEntry::id() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->id();
}

QString KeyStoreEntry::storeName() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->storeName();
}

QString KeyStoreEntry::storeId() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->storeId();
}

KeyBundle KeyStoreEntry::keyBundle() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->keyBundle();
}

Certificate KeyStoreEntry::certificate() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->certificate();
}

CRL KeyStoreEntry::crl() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->crl();
}

PGPKey KeyStoreEntry::pgpSecretKey() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->pgpSecretKey();
}

PGPKey KeyStoreEntry::pgpPublicKey() const
{
	return static_cast<const KeyStoreEntryContext *>(context())->pgpPublicKey();
}

bool KeyStoreEntry::ensureAvailable()
{
	QString storeId = this->storeId();
	QString entryId = id();
	KeyStoreEntryContext *c = (KeyStoreEntryContext *)qVariantValue<void*>(trackercall("entry", QVariantList() << storeId << entryId));
	if(c)
	{
		change(c);
		d->available = true;
		return true;
	}
	d->available = false;
	return false;
}

bool KeyStoreEntry::ensureAccess()
{
	if(!ensureAvailable())
	{
		d->accessible = false;
		return false;
	}
	bool ok = static_cast<KeyStoreEntryContext *>(context())->ensureAccess();
	d->accessible = ok;
	return d->accessible;
}

// from KeyStoreTracker
void KeyStoreTracker::setEntryAvailable(KeyStoreEntry *e)
{
	e->d->available = true;
}

//----------------------------------------------------------------------------
// KeyStoreEntryWatcher
//----------------------------------------------------------------------------
/*class KeyStoreEntryWatcher::Private : public QObject
{
	Q_OBJECT
public:
	KeyStoreEntryWatcher *q;
	KeyStoreManager ksm;
	KeyStoreEntry e;

	Private(KeyStoreEntryWatcher *_q) : q(_q)
	{
	}
};*/

KeyStoreEntryWatcher::KeyStoreEntryWatcher(const KeyStoreEntry &e, QObject *parent)
:QObject(parent)
{
	// TODO
	Q_UNUSED(e);
	//d = new Private(this);
}

KeyStoreEntryWatcher::~KeyStoreEntryWatcher()
{
	//delete d;
}

/*void KeyStoreEntryWatcher::start(const KeyStoreEntry &e)
{
	// TODO
	Q_UNUSED(e);
}

void KeyStoreEntryWatcher::stop()
{
	// TODO
}*/

KeyStoreEntry KeyStoreEntryWatcher::entry() const
{
	// TODO
	return KeyStoreEntry();
}

//----------------------------------------------------------------------------
// KeyStore
//----------------------------------------------------------------------------
class KeyStorePrivate
{
public:
	KeyStore *q;
	KeyStoreManager *ksm;
	int trackerId;
	KeyStoreTracker::Item item;

	KeyStorePrivate(KeyStore *_q) : q(_q)
	{
	}

	// implemented below, after KeyStorePrivate is declared
	void reg();
	void unreg();
	KeyStoreTracker::Item *getItem(const QString &storeId);
	KeyStoreTracker::Item *getItem(int trackerId);

	void invalidate()
	{
		trackerId = -1;
	}
};

KeyStore::KeyStore(const QString &id, KeyStoreManager *keyStoreManager)
:QObject(keyStoreManager)
{
	d = new KeyStorePrivate(this);
	d->ksm = keyStoreManager;

	KeyStoreTracker::Item *i = d->getItem(id);
	if(i)
	{
		d->trackerId = i->trackerId;
		d->item = *i;
	}
	else
		d->trackerId = -1;
}

KeyStore::~KeyStore()
{
	delete d;
}

bool KeyStore::isValid() const
{
	return (d->getItem(d->trackerId) ? true : false);
}

KeyStore::Type KeyStore::type() const
{
	return d->item.type;
}

QString KeyStore::name() const
{
	return d->item.name;
}

QString KeyStore::id() const
{
	return d->item.storeId;
}

bool KeyStore::isReadOnly() const
{
	return d->item.isReadOnly;
}

QList<KeyStoreEntry> KeyStore::entryList() const
{
	if(d->trackerId == -1)
		return QList<KeyStoreEntry>();
	return qVariantValue< QList<KeyStoreEntry> >(trackercall("entryList", QVariantList() << d->trackerId));
}

bool KeyStore::holdsTrustedCertificates() const
{
	QList<KeyStoreEntry::Type> list;
	if(d->trackerId == -1)
		return false;
	list = qVariantValue< QList<KeyStoreEntry::Type> >(trackercall("entryTypes", QVariantList() << d->trackerId));
	if(list.contains(KeyStoreEntry::TypeCertificate) || list.contains(KeyStoreEntry::TypeCRL))
		return true;
	return false;
}

bool KeyStore::holdsIdentities() const
{
	QList<KeyStoreEntry::Type> list;
	if(d->trackerId == -1)
		return false;
	list = qVariantValue< QList<KeyStoreEntry::Type> >(trackercall("entryTypes", QVariantList() << d->trackerId));
	if(list.contains(KeyStoreEntry::TypeKeyBundle) || list.contains(KeyStoreEntry::TypePGPSecretKey))
		return true;
	return false;
}

bool KeyStore::holdsPGPPublicKeys() const
{
	QList<KeyStoreEntry::Type> list;
	if(d->trackerId == -1)
		return false;
	list = qVariantValue< QList<KeyStoreEntry::Type> >(trackercall("entryTypes", QVariantList() << d->trackerId));
	if(list.contains(KeyStoreEntry::TypePGPPublicKey))
		return true;
	return false;
}

bool KeyStore::writeEntry(const KeyBundle &kb)
{
	// TODO
	Q_UNUSED(kb);
	return false;
}

bool KeyStore::writeEntry(const Certificate &cert)
{
	// TODO
	Q_UNUSED(cert);
	return false;
}

bool KeyStore::writeEntry(const CRL &crl)
{
	// TODO
	Q_UNUSED(crl);
	return false;
}

PGPKey KeyStore::writeEntry(const PGPKey &key)
{
	// TODO
	Q_UNUSED(key);
	return PGPKey();
}

bool KeyStore::removeEntry(const QString &id)
{
	// TODO
	Q_UNUSED(id);
	return false;
}

//----------------------------------------------------------------------------
// KeyStoreManager
//----------------------------------------------------------------------------
static void ensure_init()
{
	QMutexLocker locker(ksm_mutex());
	if(!g_ksm)
		g_ksm = new KeyStoreManagerGlobal;
}

// static functions
void KeyStoreManager::start()
{
	scanForPlugins();
	ensure_init();
	QMetaObject::invokeMethod(KeyStoreTracker::instance(), "start", Qt::QueuedConnection);
}

void KeyStoreManager::start(const QString &provider)
{
	ensure_init();
	QMetaObject::invokeMethod(KeyStoreTracker::instance(), "start", Qt::QueuedConnection, Q_ARG(QString, provider));
}

QString KeyStoreManager::diagnosticText()
{
	ensure_init();
	return KeyStoreTracker::instance()->getDText();
}

void KeyStoreManager::clearDiagnosticText()
{
	ensure_init();
	KeyStoreTracker::instance()->clearDText();
}

void KeyStoreManager::scan()
{
	ensure_init();
	QMetaObject::invokeMethod(KeyStoreTracker::instance(), "scan", Qt::QueuedConnection);
}

void KeyStoreManager::shutdown()
{
	QMutexLocker locker(ksm_mutex());
	delete g_ksm;
	g_ksm = 0;
}

// object
class KeyStoreManagerPrivate : public QObject
{
	Q_OBJECT
public:
	KeyStoreManager *q;

	QMutex m;
	QWaitCondition w;
	bool busy;
	QList<KeyStoreTracker::Item> items;
	bool pending, waiting;

	QMultiHash<int,KeyStore*> keyStoreForTrackerId;
	QHash<KeyStore*,int> trackerIdForKeyStore;

	KeyStoreManagerPrivate(KeyStoreManager *_q) : QObject(_q), q(_q)
	{
		pending = false;
		waiting = false;
	}

	// for keystore
	void reg(KeyStore *ks, int trackerId)
	{
		keyStoreForTrackerId.insert(trackerId, ks);
		trackerIdForKeyStore.insert(ks, trackerId);
	}

	void unreg(KeyStore *ks)
	{
		int trackerId = trackerIdForKeyStore.take(ks);

		// this is the only way I know to remove one item from a multihash
		QList<KeyStore*> vals = keyStoreForTrackerId.values(trackerId);
		keyStoreForTrackerId.remove(trackerId);
		vals.removeAll(ks);
		foreach(KeyStore *i, vals)
			keyStoreForTrackerId.insert(trackerId, i);
	}

	KeyStoreTracker::Item *getItem(const QString &storeId)
	{
		for(int n = 0; n < items.count(); ++n)
		{
			KeyStoreTracker::Item *i = &items[n];
			if(i->storeId == storeId)
				return i;
		}
		return 0;
	}

	KeyStoreTracker::Item *getItem(int trackerId)
	{
		for(int n = 0; n < items.count(); ++n)
		{
			KeyStoreTracker::Item *i = &items[n];
			if(i->trackerId == trackerId)
				return i;
		}
		return 0;
	}

	void do_update()
	{
		QPointer<QObject> self(this); // for signal-safety

		bool newbusy = KeyStoreTracker::instance()->isBusy();
		QList<KeyStoreTracker::Item> newitems = KeyStoreTracker::instance()->getItems();

		if(!busy && newbusy)
		{
			emit q->busyStarted();
			if(!self)
				return;
		}
		if(busy && !newbusy)
		{
			emit q->busyFinished();
			if(!self)
				return;
		}

		QStringList here;
		QList<int> changed;
		QList<int> gone;

		// removed
		for(int n = 0; n < items.count(); ++n)
		{
			KeyStoreTracker::Item &i = items[n];
			bool found = false;
			for(int k = 0; k < newitems.count(); ++k)
			{
				if(i.trackerId == newitems[k].trackerId)
				{
					found = true;
					break;
				}
			}
			if(!found)
				gone += i.trackerId;
		}

		// changed
		for(int n = 0; n < items.count(); ++n)
		{
			KeyStoreTracker::Item &i = items[n];
			for(int k = 0; k < newitems.count(); ++k)
			{
				if(i.trackerId == newitems[k].trackerId)
				{
					if(i.updateCount < newitems[k].updateCount)
						changed += i.trackerId;
					break;
				}
			}
		}

		// added
		for(int n = 0; n < newitems.count(); ++n)
		{
			KeyStoreTracker::Item &i = newitems[n];
			bool found = false;
			for(int k = 0; k < items.count(); ++k)
			{
				if(i.trackerId == items[k].trackerId)
				{
					found = true;
					break;
				}
			}
			if(!found)
				here += i.storeId;
		}

		// signals
		foreach(int trackerId, gone)
		{
			KeyStore *ks = keyStoreForTrackerId.value(trackerId);
			if(ks)
			{
				ks->d->invalidate();
				emit ks->unavailable();
				if(!self)
					return;
			}
		}

		foreach(int trackerId, changed)
		{
			KeyStore *ks = keyStoreForTrackerId.value(trackerId);
			if(ks)
			{
				emit ks->updated();
				if(!self)
					return;
			}
		}

		foreach(QString storeId, here)
		{
			emit q->keyStoreAvailable(storeId);
			if(!self)
				return;
		}

		busy = newbusy;
		items = newitems;
	}

public slots:
	void tracker_updated()
	{
		QMutexLocker locker(&m);
		if(!pending)
		{
			QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
			pending = true;
		}
		if(waiting && !KeyStoreTracker::instance()->isBusy())
			w.wakeOne();
	}

	void update()
	{
		m.lock();
		pending = false;
		m.unlock();

		do_update();
	}
};

// from KeyStorePrivate
void KeyStorePrivate::reg()
{
	ksm->d->reg(q, trackerId);
}

void KeyStorePrivate::unreg()
{
	ksm->d->unreg(q);
}

KeyStoreTracker::Item *KeyStorePrivate::getItem(const QString &storeId)
{
	return ksm->d->getItem(storeId);
}

KeyStoreTracker::Item *KeyStorePrivate::getItem(int trackerId)
{
	return ksm->d->getItem(trackerId);
}

KeyStoreManager::KeyStoreManager(QObject *parent)
:QObject(parent)
{
	ensure_init();
	d = new KeyStoreManagerPrivate(this);
	d->connect(KeyStoreTracker::instance(), SIGNAL(updated()),
		SLOT(tracker_updated()), Qt::DirectConnection);
	sync();
}

KeyStoreManager::~KeyStoreManager()
{
	delete d;
}

bool KeyStoreManager::isBusy() const
{
	return d->busy;
}

void KeyStoreManager::waitForBusyFinished()
{
	d->m.lock();
	d->busy = KeyStoreTracker::instance()->isBusy();
	if(d->busy)
	{
		d->waiting = true;
		d->w.wait(&d->m);
		d->waiting = false;
	}
	d->m.unlock();
}

QStringList KeyStoreManager::keyStores() const
{
	QStringList out;
	for(int n = 0; n < d->items.count(); ++n)
		out += d->items[n].storeId;
	return out;
}

void KeyStoreManager::sync()
{
	d->busy = KeyStoreTracker::instance()->isBusy();
	d->items = KeyStoreTracker::instance()->getItems();
}

}

#include "qca_keystore.moc"
