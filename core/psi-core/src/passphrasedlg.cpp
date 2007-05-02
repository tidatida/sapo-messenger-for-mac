/*
 * passphrasedlg.cpp - class to handle entering of OpenPGP passphrase
 * Copyright (C) 2003  Justin Karneges
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

#include <qlineedit.h>
#include <qradiobutton.h>
#include <qpushbutton.h>

#include "pgputil.h"
#include "passphrasedlg.h"
#include "common.h"
#include "iconwidget.h"


PassphraseDlg::PassphraseDlg(const QString& name, const QString& entryId, int requestId, QWidget *parent) : QDialog (parent), entryId_(entryId)
{
	setupUi(this);
	setModal(true);
	connect(pb_ok, SIGNAL(clicked()), SLOT(accept()));
	connect(pb_cancel, SIGNAL(clicked()), SLOT(reject()));
	setWindowTitle(tr("%1: OpenPGP Passphrase").arg(name));
	resize(minimumSize());
	addRequest(requestId);
}

void PassphraseDlg::addRequest(int id)
{
	requestIds_.append(id);
}

void PassphraseDlg::setEventHandler(QCA::EventHandler* eventHandler)
{
	eventHandler_ = eventHandler;
}

void PassphraseDlg::promptPassphrase(const QString& name, const QString& entryId, int requestId)
{
	//if (dialogs_.contains(entryId)) {
	//	PassphraseDlg* d = dialogs_[entryId];
	//	d->addRequest(requestId);
	//}
	//else {
		PassphraseDlg w(name,entryId,requestId);
		//dialogs_[entryId] = w;
		w.exec();
	//}
}


void PassphraseDlg::reject()
{
	foreach(int id, requestIds_) {
		eventHandler_->reject(id);
	}
	dialogs_.remove(entryId_);
	QDialog::reject();
}

void PassphraseDlg::accept()
{
	PGPUtil::passphrases[entryId_] = le_pass->text();
	foreach(int id, requestIds_) {
		eventHandler_->submitPassword(id,le_pass->text().toUtf8());
	}
	dialogs_.remove(entryId_);
	QDialog::accept();
}

QCA::EventHandler* PassphraseDlg::eventHandler_ = NULL;

QMap<QString,PassphraseDlg*> PassphraseDlg::dialogs_;
