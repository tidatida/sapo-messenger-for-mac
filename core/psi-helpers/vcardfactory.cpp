/*
 * vcardfactory.cpp - class for caching vCards
 * Copyright (C) 2003  Michail Pishchagin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QObject>
#include <QApplication>
#include <QMap>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>

#include "vcardfactory.h"
#include "jidutil.h"

/**
 * \brief Factory for retrieving and changing VCards.
 */
VCardFactory::VCardFactory(Client *c)
	: QObject(qApp), dictSize_(5)
{
	isFetchingMyVCard_ = false;
	hasFetchedMyVCard_ = false;
	
	setClient(c);
}

/**
 * \brief Destroys all cached VCards.
 */
VCardFactory::~VCardFactory()
{
	foreach (VCard *vcard, vcardDict_)
		delete vcard;
}

/**
 * \brief Returns the VCardFactory instance.
 */
VCardFactory* VCardFactory::instance() 
{
	if (!instance_) {
		instance_ = new VCardFactory();
	}
	return instance_;
}


/**
 * Adds a vcard to the cache (and removes other items if necessary)
 */
void VCardFactory::checkLimit(QString jid, VCard *vcard)
{
	if (vcardList_.contains(jid)) {
		vcardList_.remove(jid);
		delete vcardDict_.take(jid);
	}
	else if (vcardList_.size() > dictSize_) {
		QString j = vcardList_.takeLast();
		delete vcardDict_.take(j);
	}

	vcardDict_[jid] = vcard;
	vcardList_.push_front(jid);
}


void VCardFactory::taskFinished()
{
	JT_VCard *task = (JT_VCard *)sender();
	if ( task->success() ) {
		Jid j = task->jid();

		VCard *vcard = new VCard;
		*vcard = task->vcard();
		checkLimit(j.userHost(), vcard);

		// save vCard to disk
		QFile file ( vCardsDir_ + "/" + JIDUtil::encode(j.userHost()).lower() + ".xml" );
		file.open ( QIODevice::WriteOnly );
		QTextStream out ( &file );
		out.setEncoding ( QTextStream::UnicodeUTF8 );
		QDomDocument doc;
		doc.appendChild( vcard->toXml ( &doc ) );
		out << doc.toString(4);

		emit vcardChanged(j);
	}
}


/**
 * \brief Call this, when you need a cached vCard.
 */
const VCard* VCardFactory::vcard(const Jid &j)
{
	// first, try to get vCard from runtime cache
	if (vcardDict_.contains(j.userHost())) {
		return vcardDict_[j.userHost()];
	}
	
	// then try to load from cache on disk
	QFile file ( vCardsDir_ + "/" + JIDUtil::encode(j.userHost()).lower() + ".xml" );
	file.open (QIODevice::ReadOnly);
	QDomDocument doc;
	VCard *vcard = new VCard;
	if ( doc.setContent(&file, false) ) {
		vcard->fromXml( doc.documentElement() );
		checkLimit(j.userHost(), vcard);
		return vcard;
	}

	delete vcard;
	return 0;
}


/**
 * \brief Call this when you need to update vCard in cache.
 */
void VCardFactory::setVCard(const Jid &j, const VCard &v)
{
	VCard *vcard = new VCard;
	*vcard = v;
	checkLimit(j.userHost(), vcard);
	emit vcardChanged(j);
}


/**
 * \brief Call this when you need to retrieve fresh vCard from server (and store it in cache afterwards)
 */
JT_VCard* VCardFactory::getVCard(const Jid &jid, Task *rootTask, const QObject *obj, const char *slot, bool cacheVCard)
{
	JT_VCard *task = new JT_VCard( rootTask );
	if ( cacheVCard )
		task->connect(task, SIGNAL(finished()), this, SLOT(taskFinished()));
	task->connect(task, SIGNAL(finished()), obj, slot);
	task->get(Jid(jid.full()));
	task->go(true);
	return task;
}


void VCardFactory::setVCardsDir(const QString &vCardsDir)
{
	vCardsDir_ = vCardsDir;
	
	// Ensure that there's a vcard directory to save into
	QDir().mkpath(vCardsDir);
}


void VCardFactory::setClient(Client *c)
{
	if (client_) disconnect(client_);
	client_ = c;
	connect(c, SIGNAL(activated()), SLOT(clientActivated()));
}


void VCardFactory::clientActivated()
{
	// Reset the vcard
	myVCard_ = VCard();
}


VCard VCardFactory::selfVCard()
{
	if (!hasFetchedMyVCard_ && !isFetchingMyVCard_)
		startSelfVCardUpdate();
	
	return myVCard_;
}


void VCardFactory::startSelfVCardUpdate()
{
	if (!isFetchingMyVCard_) {
		isFetchingMyVCard_ = true;
		
		// Fetch our own vCard
		JT_VCard *myVCardFetchTask = new JT_VCard(client_->rootTask());
		
		connect(myVCardFetchTask, SIGNAL(finished()), SLOT(selfVCardTaskFinished()));
		
		myVCardFetchTask->getSelfVCard();
		myVCardFetchTask->go(true);
	}
}


void VCardFactory::selfVCardTaskFinished()
{
	JT_VCard *task = (JT_VCard *)sender();
	
	isFetchingMyVCard_ = false;
	hasFetchedMyVCard_ = true;
	
	myVCard_ = (task->success() ? task->vcard() : VCard());
	emit selfVCardChanged();
}


void VCardFactory::setSelfVCard(const VCard &myVCard)
{
	// Prevent startSelfVCardUpdate from starting tasks if we're setting the avatar
	isFetchingMyVCard_ = true;
	
	JT_VCard *myVCardSetter = new JT_VCard(client_->rootTask());
	
	connect(myVCardSetter, SIGNAL(finished()), SLOT(selfVCardTaskFinished()));
	
	myVCardSetter->set(myVCard);
	myVCardSetter->go(true);
}


void VCardFactory::resetSelfVCard()
{
	isFetchingMyVCard_ = hasFetchedMyVCard_ = false;
	myVCard_ = VCard();
	
	startSelfVCardUpdate();
}


bool VCardFactory::selfVCardIsAvailable()
{
	return hasFetchedMyVCard_;
}


VCardFactory* VCardFactory::instance_ = NULL;
