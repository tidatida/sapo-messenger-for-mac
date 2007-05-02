/*
 *  liveupdate.h
 *
 *	Copyright (C) 2006-2007 PT.COM,  All rights reserved.
 *	Author: Joao Pavao <jppavao@criticalsoftware.com>
 *
 *	For more information on licensing, read the README file.
 *	Para mais informa��es sobre o licenciamento, leia o ficheiro README.
 */

#ifndef LIVEUPDATE_H
#define LIVEUPDATE_H


#include "im.h"

using namespace XMPP;


class JT_SapoLiveUpdate : public Task
{
public:
	JT_SapoLiveUpdate(Task *parent, const Jid & to);
	~JT_SapoLiveUpdate();
	
	QString & url (void) {
		return _url;
	}
	
private:
	void onGo();
	bool take(const QDomElement &elem);
	
	Jid _jid;
	QDomElement _iq;
	QString _url;
};


#endif
