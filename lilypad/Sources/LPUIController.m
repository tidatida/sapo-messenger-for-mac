//
//  LPUIController.m
//  Lilypad
//
//	Copyright (C) 2006-2007 PT.COM,  All rights reserved.
//	Authors: Joao Pavao <jppavao@criticalsoftware.com>
//           Jason Kim <jason@512k.org>
//
//	For more information on licensing, read the README file.
//	Para mais informa��es sobre o licenciamento, leia o ficheiro README.
//
//
// The main application controller (on the Objective-C side of the pond).
//
// Although the conceptual "application controller" resides in the core (Qt/C++) half of the
// program, this is Lilypad's equivalent to a typical app controller class.
//

#import "LPUIController.h"
#import "LPRosterController.h"
#import "LPAuthorizationAlert.h"
#import "LPAvatarEditorController.h"
#import "LPChatController.h"
#import "LPEditContactController.h"
#import "LPSendSMSController.h"
#import "LPTermsOfUseController.h"
#import "LPFirstRunSetup.h"
#import "LPAccountsController.h"
#import "LPStatusMenuController.h"
#import "LPFileTransfersController.h"
#import "LPXmlConsoleController.h"
#import "LPSapoAgentsDebugWinCtrl.h"
#import "LPEventNotificationsHandler.h"
#import "CTBadge.h"
#import "LPRecentMessagesStore.h"
#import "NSxString+EmoticonAdditions.h"

#import "LPMessageCenter.h"
#import "LPMessageCenterWinController.h"

#import "LPAccount.h"
#import "LPRoster.h"
#import "LPPresenceSubscription.h"
#import "LPContact.h"
#import "LPContactEntry.h"
#import "LPChat.h"
#import "LPFileTransfer.h"
#import "LPSapoAgents.h"

#import <Sparkle/SUUpdater.h>


@implementation LPUIController


#pragma mark -
#pragma mark Initialization


+ (void)initialize
{
	// Register our NSValueTransformers
	[NSValueTransformer setValueTransformer:[[[LPStatusStringFromStatusTransformer alloc] init] autorelease]
									forName:LPStatusStringFromStatusTransformerName];
	[NSValueTransformer setValueTransformer:[[[LPStatusIconFromStatusTransformer alloc] init] autorelease]
									forName:LPStatusIconFromStatusTransformerName];
	[NSValueTransformer setValueTransformer:[[[LPPhoneNrStringFromPhoneJIDTransformer alloc] init] autorelease]
									forName:LPPhoneNrStringFromPhoneJIDTransformerName];
	[NSValueTransformer setValueTransformer:[[[LPAttributedStringWithEmoticonsTransformer alloc] init] autorelease]
									forName:LPAttributedStringWithEmoticonsTransformerName];
	
	// Load our defaults.
	NSString *defaultsPath;
	NSDictionary *defaultsDict;
	
	defaultsPath = [[NSBundle mainBundle] pathForResource:@"Defaults" ofType:@"plist"];
	defaultsDict = [NSDictionary dictionaryWithContentsOfFile:defaultsPath];
	
	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDict];
}


- init
{
	if (self = [super init]) {
		m_accountsController = [[LPAccountsController sharedAccountsController] retain];
		m_statusMenuControllers = [[NSMutableDictionary alloc] init];
		
		// Set as delegate for both the account and the account's roster
		LPAccount *account = [m_accountsController defaultAccount];
		[account setDelegate:self];
		[[account roster] setDelegate:self];
		
		
		m_messageCenter = [[LPMessageCenter alloc] initWithAccount:[m_accountsController defaultAccount]];
		
		m_authorizationAlertsByJID = [[NSMutableDictionary alloc] init];
		
		
		m_chatControllersByContact = [[NSMutableDictionary alloc] init];
		m_editContactControllersByContact = [[NSMutableDictionary alloc] init];
		m_smsSendingControllersByContact = [[NSMutableDictionary alloc] init];
	}
	return self;
}


- (void)awakeFromNib
{
	LPStatusMenuController *smc = [self sharedStatusMenuControllerForAccount:[[LPAccountsController sharedAccountsController] defaultAccount]];
	[smc insertControlledStatusItemsIntoMenu:m_statusMenu atIndex:0];
	
	// Forced disable of Spakle automated updates
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"SUCheckAtStartup"];
	
	[m_addContactSupermenu setDelegate:[self rosterController]];
}


- (void)dealloc
{
	[m_appIconBadge release];
	
	[m_prefsController release];
	[m_rosterController release];
	[m_avatarEditorController release];
	[m_fileTransfersController release];
	[m_xmlConsoleController release];
	
	[[m_accountsController defaultAccount] setDelegate:nil];
	[m_accountsController release];
	[m_statusMenuControllers release];
	
	[m_messageCenter release];
	[m_messageCenterWinController release];
	
	[m_authorizationAlertsByJID release];
	
	[m_chatControllersByContact release];
	[m_editContactControllersByContact release];
	[m_smsSendingControllersByContact release];
	
	[m_provideFeedbackURL release];

	[super dealloc];
}


- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([keyPath isEqualToString:@"online"]) {
		// Successful first-time auto-login
		LPAccount *account = object;
		NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
		
		if ([account isOnline]) {
			NSString *currentVersion = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
			[defaults setObject:currentVersion forKey:@"LastVersionRun"];
			
			[object removeObserver:self forKeyPath:@"online"];
		}
	}
	else if ([keyPath isEqualToString:@"requiresUserIntervention"]) {
		LPPresenceSubscription	*presSub = object;
		
		if (![presSub requiresUserIntervention]) {
			NSString				*JID = [[presSub contactEntry] address];
			LPAuthorizationAlert	*authAlert = [m_authorizationAlertsByJID objectForKey:JID];
			
			[authAlert close];
		}
	}
	else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (LPStatusMenuController *)sharedStatusMenuControllerForAccount:(LPAccount *)account
{
	NSString *accountUUID = [account UUID];
	LPStatusMenuController *menuController = [m_statusMenuControllers objectForKey:accountUUID];
	
	if (menuController == nil) {
		menuController = [[LPStatusMenuController alloc] initWithAccount:account];
		[m_statusMenuControllers setObject:menuController forKey:accountUUID];
	}
	
	return menuController;
}

- (LPAccountsController *)accountsController
{
	return [[m_accountsController retain] autorelease];
}


- (LPRosterController *)rosterController
{
	if (m_rosterController == nil) {
		LPRoster *roster = [[[self accountsController] defaultAccount] roster];
		m_rosterController = [[LPRosterController alloc] initWithRoster:roster delegate:self];
		
		[m_rosterController addGroupMenu:m_groupsMenu];
	}
	return m_rosterController;
}


- (LPAvatarEditorController *)avatarEditorController
{
	if (m_avatarEditorController == nil) {
		m_avatarEditorController = [[LPAvatarEditorController alloc] initWithAccount:[[self accountsController] defaultAccount]];
	}
	return m_avatarEditorController;
}


- (LPFileTransfersController *)fileTransfersController
{
	if (m_fileTransfersController == nil) {
		m_fileTransfersController = [[LPFileTransfersController alloc] init];
	}
	return m_fileTransfersController;
}


- (LPMessageCenterWinController *)messageCenterWindowController
{
	if (m_messageCenterWinController == nil) {
		m_messageCenterWinController = [[LPMessageCenterWinController alloc] initWithMessageCenter:m_messageCenter];
		[m_messageCenterWinController setDelegate:self];
	}
	return m_messageCenterWinController;
}


- (void)showWindowForChatWithContact:(LPContact *)contact
{
	LPChatController *chatCtrl = [m_chatControllersByContact objectForKey:contact];
	
	if (chatCtrl == nil && [contact canDoChat]) {
		chatCtrl = [[LPChatController alloc] initOutgoingWithContact:contact delegate:self];
		if (chatCtrl) {
			[m_chatControllersByContact setObject:chatCtrl forKey:contact];
			[chatCtrl release];
		}
	}
	
	[chatCtrl showWindow:nil];
}


- (void)showWindowForEditingContact:(LPContact *)contact
{
	LPEditContactController *editContactController = [m_editContactControllersByContact objectForKey:contact];
	
	if (editContactController == nil) {
		editContactController = [[LPEditContactController alloc] initWithContact:contact delegate:self];
		[m_editContactControllersByContact setObject:editContactController forKey:contact];
		[editContactController release];
	}
	
	[editContactController showWindow:nil];
}


- (void)showWindowForSendingSMSWithContact:(LPContact *)contact
{
	LPSendSMSController *smsCtrl = [m_smsSendingControllersByContact objectForKey:contact];
	
	if (smsCtrl == nil && [contact canDoSMS]) {
		smsCtrl = [[LPSendSMSController alloc] initWithContact:contact delegate:self];
		if (smsCtrl) {
			[m_smsSendingControllersByContact setObject:smsCtrl forKey:contact];
			[smsCtrl release];
		}
	}
	
	[smsCtrl showWindow:nil];
}


- (void)updateXMLConsoleMenuItemVisibility
{
	// Check if we should hide the "XML Console" menu item
	NSMenu *menu = [NSApp windowsMenu];
	
	UInt32 requiredFlags = (optionKey | controlKey | shiftKey);
	UInt32 currentFlags = GetCurrentKeyModifiers();
	
	if ((currentFlags & requiredFlags) == requiredFlags) {
		if (m_XMLConsoleMenuItem == nil) {
			// Show the menu item after the "File Transfers" item
			m_XMLConsoleMenuItem = [[NSMenuItem alloc] initWithTitle:@"XML Console" action:@selector(showXmlConsole:) keyEquivalent:@""];
			[menu insertItem:m_XMLConsoleMenuItem atIndex:(1 + [menu indexOfItemWithTarget:self andAction:@selector(showFileTransfers:)])];
			[m_XMLConsoleMenuItem release];
		}
		
		[self showXmlConsole:nil];
		[m_xmlConsoleController setLoggingEnabled:YES];
	}
}


#pragma mark -
#pragma mark Actions


- (IBAction)setStatusMessage:(id)sender
{
	[[self rosterController] setStatusMessage];
}

- (IBAction)showRoster:(id)sender
{
	[[self rosterController] showWindow:sender];
}


- (IBAction)showAvatarEditor:(id)sender
{
	[[self avatarEditorController] showWindow:sender];
}


- (IBAction)showFileTransfers:(id)sender
{
	[[self fileTransfersController] showWindow:nil];
}


- (IBAction)showMessageCenter:(id)sender
{
	[[self messageCenterWindowController] showWindow:nil];
}


- (IBAction)showXmlConsole:(id)sender
{
	if (m_xmlConsoleController == nil) {
		LPAccount *account = [[self accountsController] defaultAccount];
		m_xmlConsoleController = [[LPXmlConsoleController alloc] initWithAccount:account];
	}
	
	[m_xmlConsoleController showWindow:sender];
}

- (IBAction)showSapoAgentsDebugWindow:(id)sender
{
	if (m_sapoAgentsDebugWinCtrl == nil) {
		LPAccount *account = [[self accountsController] defaultAccount];
		m_sapoAgentsDebugWinCtrl = [[LPSapoAgentsDebugWinCtrl alloc] initWithAccount:account];
	}
	
	[m_sapoAgentsDebugWinCtrl showWindow:sender];
}

- (IBAction)reportBug:(id)sender
{
	NSBundle	*bundle = [NSBundle mainBundle];
	NSString	*urlFormatString = [bundle objectForInfoDictionaryKey:@"LPBugSubmissionURL"];
	id			buildNr = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
	NSString	*urlString = [NSString stringWithFormat:urlFormatString, buildNr];
	
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:urlString]];
}

- (IBAction)provideFeedback:(id)sender
{
	if (m_provideFeedbackURL) {
		[[NSWorkspace sharedWorkspace] openURL:m_provideFeedbackURL];
	}
}


/* The goal of this is to force menu validation to be performed for these selectors in order to update the
corresponding menu item titles and state to default values. We implement the methods but always disable
their menu items. */
- (IBAction)toggleShowOfflineBuddies:(id)sender { }
- (IBAction)toggleShowGroups:(id)sender { }
- (IBAction)sortByAvailability:(id)sender { }
- (IBAction)sortByName:(id)sender { }
- (IBAction)removeContacts:(id)sender { }


- (BOOL)validateMenuItem:(id <NSMenuItem>)menuItem
{
	SEL action = [menuItem action];
	BOOL enabled = NO;
	
	if (action == @selector(toggleShowOfflineBuddies:) || action == @selector(sortByName:)) {
		[menuItem setState:NSOffState];
	}
	else if (action == @selector(toggleShowGroups:) || action == @selector(sortByAvailability:)) {
		[menuItem setState:NSOnState];
	}
	else if (action == @selector(removeContacts:)) {
		[menuItem setTitle:NSLocalizedString(@"Remove Contact...", @"menu item title")];
	}
	else if (action == @selector(setStatusMessage:)) {
		enabled = [[[self accountsController] defaultAccount] isOnline];
	}
	else if (action == @selector(provideFeedback:)) {
		enabled = (m_provideFeedbackURL != nil);
	}
	else {
		enabled = YES;
	}
	
	return enabled;
}


#pragma mark -
#pragma mark NSApplication Delegate Methods


- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
	// We require Tiger or newer.
	if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_3)
	{
		NSString *appName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleExecutable"];
		NSString *title = NSLocalizedString(@"Unsupported Operating System", @"startup error");
		NSString *msg = NSLocalizedString(@"Sorry, %@ requires Mac OS X 10.4 or newer.", @"startup error");
		
		NSRunCriticalAlertPanel(title, msg, NSLocalizedString(@"OK", @""), nil, nil, appName);
		[[notification object] terminate:self];
	}
}


- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	// Check if we should show the "XML Console" menu item
	[self updateXMLConsoleMenuItemVisibility];
	
	// Install the debug menu
	if ([defaults boolForKey:@"IncludeDebugMenu"]) {
		[m_debugMenu setTitle:@"Debug"];
		NSMenuItem *debugMenuItem = [[NSApp mainMenu] addItemWithTitle:@"Debug" action:NULL keyEquivalent:@""];
		[debugMenuItem setSubmenu:m_debugMenu];
	}
	
	[LPEventNotificationsHandler registerWithGrowl];
	[[LPEventNotificationsHandler defaultHandler] setDelegate:self];
	
	// Start by initializing some stuff on the bridge
	NSTimeZone	*tz = [NSTimeZone defaultTimeZone];
	NSBundle	*appBundle = [NSBundle mainBundle];
	NSString	*clientName = [NSString stringWithFormat:@"%@ Mac", [appBundle objectForInfoDictionaryKey:@"CFBundleExecutable"]];
	NSString	*versionString = [NSString stringWithFormat:@"%@ (%@)",
		[appBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"],
		[appBundle objectForInfoDictionaryKey:@"CFBundleVersion"]];
	NSString	*capsVersionString = [NSString stringWithFormat:@"%@_%@",
		[appBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"],
		[appBundle objectForInfoDictionaryKey:@"CFBundleVersion"]];
	
	[LFAppController setTimeZoneName:[tz abbreviation] timeZoneOffset:([tz secondsFromGMT] / 3600)];
	[LFAppController setClientName:clientName
						   version:versionString
							OSName:@"Mac OS X"
						  capsNode:@"http://messenger.sapo.pt/caps/mac"
					   capsVersion:capsVersionString];
	[LFAppController setSupportDataFolder: LPOurApplicationSupportFolderPath()];
	
	
	// Build number dependent stuff
	NSString *lastVersionRun = [defaults stringForKey:@"LastVersionRun"];
	NSString *currentVersion = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
	
	if ([lastVersionRun intValue] < [currentVersion intValue]) {
		// This is the first run of this version
		BOOL termsOfUseAccepted = [[LPTermsOfUseController termsOfUse] runModal];
		if (termsOfUseAccepted == NO)
			[NSApp terminate:nil];
	}
	
	if (lastVersionRun == nil) {
		// This is the first run of the application
		[[LPFirstRunSetup firstRunSetup] runModal];
		[[[self accountsController] defaultAccount] addObserver:self forKeyPath:@"online" options:0 context:NULL];
	}
	else {
		[defaults setObject:currentVersion forKey:@"LastVersionRun"];
	}
	
	
	[self showRoster:nil];
	[[self accountsController] connectAllAutologinAccounts:nil];
}


- (void)applicationDidBecomeActive:(NSNotification *)aNotification
{
	m_totalNrOfUnreadMessages = 0;
	[NSApp setApplicationIconImage:[NSImage imageNamed:@"NSApplicationIcon"]];
}


- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
	if (flag == NO) {
		[self showRoster:nil];
	}
	return flag;
}


- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	[[self accountsController] disconnectAllAccounts:nil];
	[LFAppController systemQuit];
	return NSTerminateLater;
}


- (void)confirmPendingTermination:(id)arg
{
	BOOL shouldTerminate = [(NSNumber *)arg boolValue];
	[NSApp replyToApplicationShouldTerminate: shouldTerminate];
}


#pragma mark -
#pragma mark LPAccount Delegate Methods


- (void)accountWillChangeStatus:(NSNotification *)notif
{
	LPAccount *account = [notif object];
	LPStatus newStatus = [[[notif userInfo] objectForKey:@"NewStatus"] intValue];
	
	BOOL willBeOnline = ((newStatus != LPStatusOffline) && (newStatus != LPStatusConnecting));
	
	if (![account isOnline] && willBeOnline) {
		[[LPRecentMessagesStore sharedMessagesStore] setOurAccountJID:[account JID]];
	}
	
	if ([account isOffline] && newStatus == LPStatusConnecting) {
		[self updateXMLConsoleMenuItemVisibility];
	}
}


- (void)account:(LPAccount *)account didReceiveErrorNamed:(NSString *)errorName errorKind:(int)errorKind errorCode:(int)errorCode
{
	NSAlert *alert;
	
#warning Limpar dos ficheiros "*.string" as localized strings todas que deixaram de ser usadas no codigo.
	
	NSString *alertTitle = NSLocalizedStringFromTable([errorName stringByAppendingString:@"_Title"], @"ConnectionError", @"");
	NSString *alertMsg = NSLocalizedStringFromTable([errorName stringByAppendingString:@"_Msg"], @"ConnectionError", @"");
	
	alert = [NSAlert alertWithMessageText:alertTitle
							defaultButton:NSLocalizedString(@"OK", @"")
						  alternateButton:nil
							  otherButton:nil
				informativeTextWithFormat:@"%@\n\n(Error code: %d:%d)", alertMsg, errorKind, errorCode];
	
	[alert runModal];
	
	// Was this the first-time login?
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	
	if ([defaults stringForKey:@"LastVersionRun"] == nil) {
		// this is the first run of the application
		[[[self rosterController] window] orderOut:nil];
		[[LPFirstRunSetup firstRunSetup] runModal];
		
		[self showRoster:nil];
		[[self accountsController] connectAllAutologinAccounts:nil];
	}
}


- (void)account:(LPAccount *)account didReceiveIncomingChat:(LPChat *)newChat
{
	NSAssert(([m_chatControllersByContact objectForKey:[newChat contact]] == nil),
			 @"There is already a chat controller for this contact");
	
	LPChatController *chatCtrl = [[LPChatController alloc] initWithIncomingChat:newChat delegate:self];
	[m_chatControllersByContact setObject:chatCtrl forKey:[newChat contact]];
	[chatCtrl release];
	
	[chatCtrl showWindow:nil];
}


- (void)p_processNewFileTransfer:(LPFileTransfer *)newFileTransfer
{
	// Chat window
	LPContact			*contact = [[newFileTransfer peerContactEntry] contact];
	LPChatController	*chatController = [m_chatControllersByContact objectForKey:contact];
	
	if (chatController == nil) {
		[self showWindowForChatWithContact:contact];
		chatController = [m_chatControllersByContact objectForKey:contact];
	}
	[chatController updateInfoForFileTransfer:newFileTransfer];
	
	// File Transfers window
	LPFileTransfersController *ftController = [self fileTransfersController];
	NSWindow *ftWin = [ftController window];
	NSWindow *keyWin = [NSApp keyWindow];
	
	if (![ftWin isVisible]) {
		if (keyWin) {
			[ftWin orderWindow:NSWindowBelow relativeTo:[keyWin windowNumber]];
		} else {
			[ftWin orderFront:nil];
		}
	}
	
	[[self fileTransfersController] addFileTransfer:newFileTransfer];
}


- (void)account:(LPAccount *)account didReceiveIncomingFileTransfer:(LPFileTransfer *)newFileTransfer
{
	[self p_processNewFileTransfer:newFileTransfer];
}


- (void)account:(LPAccount *)account willStartOutgoingFileTransfer:(LPFileTransfer *)newFileTransfer
{
	[self p_processNewFileTransfer:newFileTransfer];
}


- (void)account:(LPAccount *)account didReceiveLiveUpdateURL:(NSString *)URLString
{
	[[NSUserDefaults standardUserDefaults] setObject:URLString forKey:@"SUFeedURL"];
	[m_appUpdater checkForUpdatesInBackground];
}


- (void)account:(LPAccount *)account didReceiveServerVarsDictionary:(NSDictionary *)varsValues
{
	// "Provide Feedback" URL
	NSString *provideFeedbackValue = [varsValues objectForKey:@"url.mac.feedback"];
	
	if ([provideFeedbackValue length] > 0) {
		NSBundle	*bundle = [NSBundle mainBundle];
		id			buildNr = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
		
		NSMutableString *urlString = [NSMutableString stringWithString:provideFeedbackValue];
		
		[urlString replaceOccurrencesOfString:@"SVN_BUILD_NUMBER"
								   withString:[buildNr description]
									  options:NSLiteralSearch
										range:NSMakeRange(0, [urlString length])];
		
		[m_provideFeedbackURL release];
		m_provideFeedbackURL = [[NSURL URLWithString:urlString] retain];
	}
	
	// Sapo Notifications Manager URL
	NSString *sapoNotificationsManagerValue = [varsValues objectForKey:@"url.community.notification_manager"];
	
	if ([sapoNotificationsManagerValue length] > 0) {
		NSURL *managerURL = [NSURL URLWithString:sapoNotificationsManagerValue];
		[[self messageCenterWindowController] setSapoNotificationsManagerURL:managerURL];
	}
}


- (void)account:(LPAccount *)account didReceiveOfflineMessageFromJID:(NSString *)fromJID nick:(NSString *)nick timestamp:(NSString *)timestamp subject:(NSString *)subject plainTextVariant:(NSString *)plainTextVariant XHTMLVariant:(NSString *)xhtmlVariant URLs:(NSArray *)urls
{
	[m_messageCenter addReceivedOfflineMessageFromJID:(NSString *)fromJID nick:nick timestamp:timestamp subject:subject plainTextVariant:plainTextVariant XHTMLVariant:xhtmlVariant URLs:urls];
}


- (void)account:(LPAccount *)account didReceiveHeadlineNotificationMessageFromChannel:(NSString *)channelName subject:(NSString *)subject body:(NSString *)body itemURL:(NSString *)itemURL flashURL:(NSString *)flashURL iconURL:(NSString *)iconURL
{
	[m_messageCenter addReceivedSapoNotificationFromChannel:channelName subject:subject body:body
													itemURL:itemURL flashURL:flashURL iconURL:iconURL];
}


#pragma mark -
#pragma mark LPRoster Delegate Methods


- (void)roster:(LPRoster *)roster didReceivePresenceSubscriptionRequest:(LPPresenceSubscription *)presSub
{
	// Add to message center
	[m_messageCenter addReceivedPresenceSubscription:presSub];
	
	// Show it on its own window/alert
	LPPresenceSubscriptionState state = [presSub state];
	LPContactEntry *entry = [presSub contactEntry];
	
	if (state == LPAuthorizationRequested)
	{
		LPAuthorizationAlert *authAlert = [m_authorizationAlertsByJID objectForKey:[entry address]];
		
		if (authAlert) {
			[[authAlert window] makeKeyAndOrderFront:nil];
		}
		else {
			authAlert = [LPAuthorizationAlert authorizationAlert];
			
			[authAlert setMessageText:[NSString stringWithFormat:
				NSLocalizedString(@"Authorize \"%@\" to see your online status?", @"presence subscription alert"),
				[entry humanReadableAddress]]];
			[authAlert setInformativeText:[NSString stringWithFormat:
				NSLocalizedString(@"The contact with the address \"%@\" has added your address to their contact list and wants to ask "
								  @"for your authorization to see when you are online. Do you want to allow this person to see your "
								  @"online status?", @"presence subscription alert"),
				[entry humanReadableAddress]]];
			[authAlert setFirstButtonTitle:NSLocalizedString(@"Authorize", @"presence subscription alert")];
			[authAlert setSecondButtonTitle:NSLocalizedString(@"Don't Authorize", @"presence subscription alert")];
			
			[m_authorizationAlertsByJID setObject:authAlert forKey:[entry address]];
			[presSub addObserver:self forKeyPath:@"requiresUserIntervention" options:0 context:NULL];
			
			[authAlert showWindowWithDelegate:self
							   didEndSelector:@selector(authorizationRequestAlertDidEnd:returnCode:contextInfo:)
								  contextInfo:(void *)[presSub retain]
									  makeKey:NO];
		}
	}
	else if (state == LPAuthorizationLost)
	{
		LPAuthorizationAlert *authAlert = [m_authorizationAlertsByJID objectForKey:[entry address]];
		
		if (authAlert) {
			[[authAlert window] makeKeyAndOrderFront:nil];
		}
		else {
			authAlert = [LPAuthorizationAlert authorizationAlert];
			
			[authAlert setMessageText:[NSString stringWithFormat:
				NSLocalizedString(@"Authorization to see the online status of \"%@\" was denied!", @"presence subscription alert"),
				[entry humanReadableAddress]]];
			
			if ([[entry humanReadableAddress] isEqualToString:[[entry contact] name]]) {
				[authAlert setInformativeText:[NSString stringWithFormat:
					NSLocalizedString(@"Your authorization to see the online status of the address \"%@\" has been denied. "
									  @"You may either remove this address from your contact list or try to renew the authorization.", @"presence subscription alert"),
					[entry humanReadableAddress]]];
			}
			else {
				[authAlert setInformativeText:[NSString stringWithFormat:
					NSLocalizedString(@"The contact \"%@\" has denied your authorization to see the online status of the address \"%@\". "
									  @"You may either remove this address from your contact list or try to renew the authorization.", @"presence subscription alert"),
					[[entry contact] name],
					[entry humanReadableAddress]]];
			}
			[authAlert setFirstButtonTitle:NSLocalizedString(@"Remove Address", @"presence subscription alert")];
			[authAlert setSecondButtonTitle:NSLocalizedString(@"Renew", @"presence subscription alert")];
			
			[m_authorizationAlertsByJID setObject:authAlert forKey:[entry address]];
			[presSub addObserver:self forKeyPath:@"requiresUserIntervention" options:0 context:NULL];
			
			[authAlert showWindowWithDelegate:self
							   didEndSelector:@selector(authorizationLostAlertDidEnd:returnCode:contextInfo:)
								  contextInfo:(void *)[presSub retain]
									  makeKey:NO];
		}
	}
}

- (void)authorizationRequestAlertDidEnd:(LPAuthorizationAlert *)alert returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	LPPresenceSubscription *presSub = [(LPPresenceSubscription *)contextInfo autorelease];
	
	[presSub removeObserver:self forKeyPath:@"requiresUserIntervention"];
	[m_authorizationAlertsByJID removeObjectForKey:[[presSub contactEntry] address]];
	
	if (returnCode == NSAlertFirstButtonReturn) {
		[presSub approveRequest];
	}
	else if (returnCode == NSAlertSecondButtonReturn) {
		[presSub rejectRequest];
	}
}

- (void)authorizationLostAlertDidEnd:(LPAuthorizationAlert *)alert returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	LPPresenceSubscription *presSub = [(LPPresenceSubscription *)contextInfo autorelease];
	
	[presSub removeObserver:self forKeyPath:@"requiresUserIntervention"];
	[m_authorizationAlertsByJID removeObjectForKey:[[presSub contactEntry] address]];
	
	if (returnCode == NSAlertFirstButtonReturn) {
		[presSub removeContactEntry];
	}
	else if (returnCode == NSAlertSecondButtonReturn) {
		[presSub sendRequest];
	}
}


#pragma mark -


- (NSMenu *)p_menuForAddingJIDsWithAction:(SEL)action
{
	// Create the popup menu
	NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Add Contact Menu"];
	LPSapoAgents *sapoAgents = [[m_accountsController defaultAccount] sapoAgents];
	NSDictionary *sapoAgentsDict = [sapoAgents dictionaryRepresentation];
	NSArray *rosterContactHostnames = [sapoAgents rosterContactHostnames];
	
	id <NSMenuItem> item;
	NSEnumerator *hostnameEnum = [rosterContactHostnames objectEnumerator];
	NSString *hostname;
	while (hostname = [hostnameEnum nextObject]) {
		item = [menu addItemWithTitle:[[sapoAgentsDict objectForKey:hostname] objectForKey:@"name"]
							   action:action
						keyEquivalent:@""];
		[item setRepresentedObject:hostname];
	}
	
	item = [menu addItemWithTitle:NSLocalizedString(@"Other Jabber Service", @"")
						   action:action
					keyEquivalent:@""];
	[item setRepresentedObject:@""];
	
	return [menu autorelease];
}


#pragma mark -
#pragma mark LPRosterController Delegate Methods


- (void)rosterController:(LPRosterController *)rosterCtrl openChatWithContact:(LPContact *)contact
{
	[self showWindowForChatWithContact:contact];
}


- (void)rosterController:(LPRosterController *)rosterCtrl sendSMSToContact:(LPContact *)contact
{
	[self showWindowForSendingSMSWithContact:contact];
}


- (void)rosterController:(LPRosterController *)rosterCtrl editContacts:(NSArray *)contacts
{
	NSEnumerator *contactEnumerator = [contacts objectEnumerator];
	LPContact *contact;
	
	while (contact = [contactEnumerator nextObject]) {
		[self showWindowForEditingContact:contact];
	}
}


- (void)rosterController:(LPRosterController *)rosterCtrl importAvatarFromPasteboard:(NSPasteboard *)pboard
{
	[[self avatarEditorController] importAvatarFromPasteboard:pboard];
	[self showAvatarEditor:self];
}


- (NSMenu *)rosterController:(LPRosterController *)rosterCtrl menuForAddingJIDsWithAction:(SEL)action
{
	return [self p_menuForAddingJIDsWithAction:action];
}


- (LPStatusMenuController *)rosterController:(LPRosterController *)rosterCtrl statusMenuControllerForAccount:(LPAccount *)account
{
	return [self sharedStatusMenuControllerForAccount:account];
}


#pragma mark -
#pragma mark LPEditContactController Delegate Methods


- (void)editContactControllerWindowWillClose:(LPEditContactController *)ctrl
{
	[m_editContactControllersByContact removeObjectForKey:[ctrl contact]];
}


- (void)editContactController:(LPEditContactController *)ctrl editContact:(LPContact *)contact
{
	[self showWindowForEditingContact:contact];
}


- (NSMenu *)editContactController:(LPEditContactController *)ctrl menuForAddingJIDsWithAction:(SEL)action
{
	return [self p_menuForAddingJIDsWithAction:action];
}


#pragma mark -
#pragma mark LPChatController Delegate Methods


- (void)chatController:(LPChatController *)chatCtrl editContact:(LPContact *)contact
{
	[self showWindowForEditingContact:contact];
}


- (void)chatController:(LPChatController *)chatCtrl sendSMSToContact:(LPContact *)contact
{
	[self showWindowForSendingSMSWithContact:contact];
}


- (void)chatControllerWindowWillClose:(LPChatController *)chatCtrl
{
	[m_chatControllersByContact removeObjectForKey:[chatCtrl contact]];
}


- (void)chatControllerDidReceiveNewMessage:(LPChatController *)chatCtrl
{
	if (![NSApp isActive]) {
		if (m_appIconBadge == nil)
			m_appIconBadge = [[CTBadge alloc] init];
		
		++m_totalNrOfUnreadMessages;
		[m_appIconBadge badgeApplicationDockIconWithValue:m_totalNrOfUnreadMessages insetX:0.0 y:0.0];
	}
}


#pragma mark -
#pragma mark LPSendSMSController Delegate Methods


- (void)smsControllerWindowWillClose:(LPSendSMSController *)smsCtrl
{
	[m_smsSendingControllersByContact removeObjectForKey:[smsCtrl contact]];
}


#pragma mark -
#pragma mark LPEventNotificationsHandler Delegate Methods


- (void)notificationsHandler:(LPEventNotificationsHandler *)handler userDidClickNotificationForContactWithID:(unsigned int)contactID
{
	[NSApp activateIgnoringOtherApps:YES];
	
	LPRoster *roster = [[self rosterController] roster];
	LPContact *contact = [roster contactForID:contactID];
	
	if (contact != nil) {
		LPChatController *existingChatController = [m_chatControllersByContact objectForKey:contact];
		
		if ([contact canDoChat] || existingChatController != nil) {
			[self showWindowForChatWithContact:contact];
		} else if ([contact canDoSMS]) {
			[self showWindowForSendingSMSWithContact:contact];
		}
	}
}


- (void)notificationsHandler:(LPEventNotificationsHandler *)handler userDidClickNotificationForHeadlineMessageWithURI:(NSString *)messageURI
{
	[NSApp activateIgnoringOtherApps:YES];
	
	if (messageURI != nil) {
		LPMessageCenterWinController *mc = [self messageCenterWindowController];
		
		[mc showWindow:nil];
		[mc revealSapoNotificationWithURI:messageURI];
	}
}


- (void)notificationsHandlerUserDidClickNotificationForOfflineMessages:(LPEventNotificationsHandler *)handler
{
	[NSApp activateIgnoringOtherApps:YES];
	
	LPMessageCenterWinController *mc = [self messageCenterWindowController];
	
	[mc showWindow:nil];
	[mc revealOfflineMessages];
}


- (void)notificationsHandlerUserDidClickNotificationForPresenceSubscriptions:(LPEventNotificationsHandler *)handler
{
	[NSApp activateIgnoringOtherApps:YES];
	
	LPMessageCenterWinController *mc = [self messageCenterWindowController];
	
	[mc showWindow:nil];
	[mc revealPresenceSubscriptions];
}


#pragma mark -
#pragma mark LPMessageCenterWinController Delegate Methods


- (void)messageCenterWinCtrl:(LPMessageCenterWinController *)mesgCenterCtrl openNewChatWithJID:(NSString *)jid
{
	LPRoster *roster = [[self rosterController] roster];
	
	LPContactEntry *contactEntry = [roster contactEntryForAddress:jid];
	LPContact *contact = [contactEntry contact];
	
	if (contact != nil) {
		[self showWindowForChatWithContact:contact];
	}
	else {
		NSRunAlertPanel(@"Unable to start chat!",
						@"Chatting with contacts that are not on your Buddy List is currently not supported",
						@"OK", nil, nil);
	}
}


@end
