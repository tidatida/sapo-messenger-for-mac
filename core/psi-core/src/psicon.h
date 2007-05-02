/*
 * psicon.h - core of Psi
 * Copyright (C) 2001, 2002  Justin Karneges
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

#ifndef PSICON_H
#define PSICON_H

#include <Q3PtrList>
#include <QList>

#include "profiles.h"
#include "psievent.h"

using namespace XMPP;

class PsiCon;
class PsiAccount;
class ContactView;
class EventDlg;
class UserListItem;
class EDB;
class EDBItem;
class ProxyManager;
class QMenuBar;
class FileTransDlg;
class IconSelectPopup;
class QThread;
class PsiActionList;
class PsiToolBar;
class TabDlg;
class AccountsComboBox;
class ChatDlg;
class TuneController;
class PsiContactList;
class Q3DockWindow;
namespace OpenPGP {
	class Engine;
}
namespace QCA {
	class Event;
}
namespace XMPP {
	class Jid;
}

class PsiCon : public QObject
{
	Q_OBJECT
public:
	enum { QuitProgram, QuitProfile };
	PsiCon();
	~PsiCon();

	bool init();
	void deinit();

	PsiContactList* contactList() const;
	ContactView *contactView() const;
	EDB *edb() const;
	TuneController* tuneController() const;
	ProxyManager *proxy() const;
	FileTransDlg *ftdlg() const;

	TabDlg* getTabs();
	TabDlg*	newTabs();
	bool isChatTabbed(ChatDlg*);
	bool isChatActiveWindow(ChatDlg*);
	ChatDlg* getChatInTabs(QString);
	TabDlg* getManagingTabs(ChatDlg*);
	Q3PtrList<TabDlg>* getTabSets();

	QWidget *dialogFind(const char *className);
	void dialogRegister(QWidget *w);
	void dialogUnregister(QWidget *w);

	QMenuBar* defaultMenuBar() const;

	void createAccount(const QString &name, const Jid &j="", const QString &pass="", bool opt_host=false, const QString &host="", int port=5222, bool legacy_ssl_probe = true, UserAccount::SSLFlag ssl=UserAccount::SSL_Auto, int proxy=0);
	PsiAccount *createAccount(const UserAccount &);
	//void createAccount(const QString &, const QString &host="", int port=5222, bool ssl=false, const QString &user="", const QString &pass="");
	void removeAccount(PsiAccount *);

	void playSound(const QString &);
	void raiseMainwin();

	AccountsComboBox *accountsComboBox(QWidget *parent=0, bool online_only = false);

	const QStringList & recentGCList() const;
	void recentGCAdd(const QString &);
	const QStringList & recentBrowseList() const;
	void recentBrowseAdd(const QString &);
	const QStringList & recentNodeList() const;
	void recentNodeAdd(const QString &);

	EventDlg *createEventDlg(const QString &, PsiAccount *);
	void updateContactGlobal(PsiAccount *, const Jid &);

	QList<PsiToolBar*> toolbarList() const;
	PsiToolBar *findToolBar(QString group, int index);
	PsiActionList *actionList() const;

	void buildToolbars();
	bool getToolbarLocation(Q3DockWindow* dw, Qt::Dock& dock, int& index, bool& nl, int& extraOffset) const;

	IconSelectPopup *iconSelectPopup() const;
	void processEvent(PsiEvent *);

signals:
	void quit(int);
	void accountAdded(PsiAccount *);
	void accountUpdated(PsiAccount *);
	void accountRemoved(PsiAccount *);
	void accountCountChanged();
	void accountActivityChanged();
	void emitOptionsUpdate();
	void pgpKeysUpdated();

public slots:
	void setGlobalStatus(const Status &, bool withPriority = false);
	void doToolbars();
	void checkAccountsEmpty();

public slots:
	void doSleep();
	void doWakeup();
	void closeProgram();
	void changeProfile();
	void doManageAccounts();
	void doGroupChat();
	void doNewBlankMessage();
	void doOptions();
	void doFileTransDlg();
	void statusMenuChanged(int);
	void pa_updatedActivity();
	void pa_updatedAccount();
	void slotApplyOptions(const Options &);
	void queueChanged();
	void recvNextEvent();
	void setStatusFromDialog(const XMPP::Status &, bool withPriority);
	void pgp_keysUpdated();
	void keyStoreAvailable(const QString&);
	void proxy_settingsChanged();
	void updateMainwinStatus();
	void tabDying(TabDlg*);
	void qcaEvent(int, const QCA::Event&);

	void mainWinGeomChanged(QRect saveableGeometry);

private slots:
	void saveAccounts();
	void optionsUpdate();

private:
	class Private;
	Private *d;

	void deleteAllDialogs();
	void s5b_init();
	void updateS5BServerAddresses();
	void setShortcuts();

	friend class PsiAccount; // FIXME
	void setToggles(bool tog_offline, bool tog_away, bool tog_agents, bool tog_hidden, bool tog_self);
	void getToggles(bool *tog_offline, bool *tog_away, bool *tog_agents, bool *tog_hidden, bool *tog_self);

	friend class EventQueue;
	int getId();
};

#endif
