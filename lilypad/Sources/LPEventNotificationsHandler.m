//
//  LPEventNotificationsHandler.m
//  Lilypad
//
//	Copyright (C) 2006-2007 PT.COM,  All rights reserved.
//	Author: Joao Pavao <jppavao@criticalsoftware.com>
//
//	For more information on licensing, read the README file.
//	Para mais informa��es sobre o licenciamento, leia o ficheiro README.
//

#import "LPEventNotificationsHandler.h"
#import "LPContact.h"
#import "LPPresenceSubscription.h"


// Notification Names for Growl
static NSString *LPContactAvailabilityChangedNotificationName	= @"Contact Availability Changed";
static NSString *LPFirstChatMessageReceivedNotificationName		= @"First Chat Message Received";
static NSString *LPChatMessageReceivedNotificationName			= @"Chat Message Received";
static NSString *LPHeadlineMessageReceivedNotificationName		= @"Notification Headline Received";
static NSString *LPOfflineMessagesReceivedNotificationName		= @"Offline Messages Received";
static NSString *LPPresenceSubscriptionReceivedNotificationName	= @"Presence Subscription Received";


// Key names for the Growl click context
static NSString *LPClickContextKindKey			= @"Kind";
static NSString *LPClickContextContactIDKey		= @"ContactID";
static NSString *LPClickContextMessageURIKey	= @"MessageURI";

static NSString *LPClickContextContactKindValue					= @"Contact";
static NSString *LPClickContextHeadlineMessageKindValue			= @"HeadlineMessage";
static NSString *LPClickContextOfflineMessagesKindValue			= @"OfflineMessages";
static NSString *LPClickContextPresenceSubscriptionKindValue	= @"PresenceSubscription";


@implementation LPEventNotificationsHandler

+ (void)registerWithGrowl
{
	// This forces the LPEventNotificationsHandler to initialize and register with Growl
	[LPEventNotificationsHandler defaultHandler];
}

+ defaultHandler
{
	static LPEventNotificationsHandler *defaultHandler = nil;

	if (defaultHandler == nil) {
		defaultHandler = [[LPEventNotificationsHandler alloc] init];
		[GrowlApplicationBridge setGrowlDelegate:defaultHandler];
	}
	return defaultHandler;
}


- (void)dealloc
{
	[m_contactAvailabilityNotificationsReenableDate release];
	[super dealloc];
}


- (id)delegate
{
	return m_delegate;
}


- (void)setDelegate:(id)delegate
{
	m_delegate = delegate;
}


- (id)p_clickContextForContact:(LPContact *)contact
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		LPClickContextContactKindValue, LPClickContextKindKey,
		[NSNumber numberWithInt:[contact ID]], LPClickContextContactIDKey,
		nil];
}

- (id)p_clickContextForHeadlineMessage:(id)message
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		LPClickContextHeadlineMessageKindValue, LPClickContextKindKey,
		[[[message objectID] URIRepresentation] absoluteString], LPClickContextMessageURIKey,
		nil];
}

- (id)p_clickContextForOfflineMessages
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		LPClickContextOfflineMessagesKindValue, LPClickContextKindKey,
		nil];
}

- (id)p_clickContextForPresenceSubscription:(id)presSub
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		LPClickContextPresenceSubscriptionKindValue, LPClickContextKindKey,
		nil];
}


#pragma mark Public Methods to Request Posting of Notifications


- (void)disableContactAvailabilityNotificationsUntilDate:(NSDate *)date
{
	[m_contactAvailabilityNotificationsReenableDate release];
	m_contactAvailabilityNotificationsReenableDate = [date retain];
}


- (void)notifyContactAvailabilityDidChange:(LPContact *)contact
{
	BOOL	shouldNotify = YES;
	
	if (m_contactAvailabilityNotificationsReenableDate != nil) {
		NSDate *now = [NSDate date];
		
		if ([now compare:m_contactAvailabilityNotificationsReenableDate] == NSOrderedAscending) {
			// We're still in the time interval where notifications should be disabled.
			shouldNotify = NO;
		}
		else {
			// Clear the "timeout" date
			[m_contactAvailabilityNotificationsReenableDate release];
			m_contactAvailabilityNotificationsReenableDate = nil;
		}
	}
	
	
	if (shouldNotify) {
		// Allow a small delay to avoid having everything happen at the same time. This also allows the avatar to be updated
		// (if it exists) from cache before actually displaying the notification.
		[self performSelector:@selector(p_displayContactAvailabilityDidChangeNotification:)
				   withObject:contact
				   afterDelay:0.5];
	}
}


- (void)p_displayContactAvailabilityDidChangeNotification:(LPContact *)contact
{
	// Avoid notifying about just added (in the last n seconds) and just deleted contacts
	if ([contact roster] != nil && [[contact creationDate] timeIntervalSinceNow] < (-10.0)) {
		NSString *description = ( ([contact status] == LPStatusOffline) ?
								  NSLocalizedString(@"went Offline", @"contact availability change notifications") :
								  NSLocalizedString(@"is now Online", @"contact availability change notifications")  );
		// Make "contact went offline" notifications non-clickable
		id clickContext = ( ([contact status] == LPStatusOffline) ?
							nil :
							[self p_clickContextForContact:contact] );
		
		[GrowlApplicationBridge notifyWithTitle:[contact name]
									description:description
							   notificationName:LPContactAvailabilityChangedNotificationName
									   iconData:[[contact avatar] TIFFRepresentation]
									   priority:0
									   isSticky:NO
								   clickContext:clickContext];
	}
}


- (void)notifyReceptionOfFirstMessage:(NSString *)message fromContact:(LPContact *)contact
{
	NSString *title = [NSString stringWithFormat:
		NSLocalizedString(@"First Message from %@", @"chat messages notifications"),
		[contact name]];
	
	NSNumber *contactIDNr = [NSNumber numberWithInt:[contact ID]];
	NSString *identifier = [[contactIDNr stringValue] stringByAppendingString:message];
	NSData *iconData = [[contact avatar] TIFFRepresentation];
	
	/*
	 * Shoot the two kinds of notifications coalesced under the same identifier. This way, if the user
	 * has "first message notifications" disabled in the prefs but regular "message notifications" enabled,
	 * then the first message will still be able to have a notification displayed.
	 */
	
	[GrowlApplicationBridge notifyWithTitle:title
								description:message
						   notificationName:LPChatMessageReceivedNotificationName
								   iconData:iconData
								   priority:1
								   isSticky:NO
							   clickContext:[self p_clickContextForContact:contact]
								 identifier:identifier];
	
	[GrowlApplicationBridge notifyWithTitle:title
								description:message
						   notificationName:LPFirstChatMessageReceivedNotificationName
								   iconData:iconData
								   priority:1
								   isSticky:NO
							   clickContext:[self p_clickContextForContact:contact]
								 identifier:identifier];
}


- (void)notifyReceptionOfMessage:(NSString *)message fromContact:(LPContact *)contact
{
	NSString *title = [NSString stringWithFormat:NSLocalizedString(@"Message from %@", @"chat messages notifications"),
		[contact name]];
	
	[GrowlApplicationBridge notifyWithTitle:title
								description:message
						   notificationName:LPChatMessageReceivedNotificationName
								   iconData:[[contact avatar] TIFFRepresentation]
								   priority:1
								   isSticky:NO
							   clickContext:[self p_clickContextForContact:contact]];
}


- (void)notifyReceptionOfHeadlineMessage:(id)message
{
	NSString *title = [NSString stringWithFormat:NSLocalizedString(@"Notification Headline", @"messages notifications")];
	
	[GrowlApplicationBridge notifyWithTitle:title
								description:[message valueForKey:@"subject"]
						   notificationName:LPHeadlineMessageReceivedNotificationName
								   iconData:nil
								   priority:1
								   isSticky:NO
							   clickContext:[self p_clickContextForHeadlineMessage:message]];
}


- (void)p_notifyReceptionOfOfflineMessages
{
	NSString *title = [NSString stringWithFormat:NSLocalizedString(@"Offline Messages", @"messages notifications")];
	NSString *descr = [NSString stringWithFormat:NSLocalizedString(@"You have received %d messages while you were offline.", @"messages notifications"), m_nrOfOfflineMessagesForDelayedNotification];
	
	[GrowlApplicationBridge notifyWithTitle:title
								description:descr
						   notificationName:LPOfflineMessagesReceivedNotificationName
								   iconData:nil
								   priority:1
								   isSticky:NO
							   clickContext:[self p_clickContextForOfflineMessages]];
	
	m_nrOfOfflineMessagesForDelayedNotification = 0;
}

- (void)notifyReceptionOfOfflineMessage:(id)message
{
	if (m_nrOfOfflineMessagesForDelayedNotification == 0) {
		[self performSelector:@selector(p_notifyReceptionOfOfflineMessages) withObject:nil afterDelay:3.0];
	}
	++m_nrOfOfflineMessagesForDelayedNotification;
}


- (void)notifyReceptionOfPresenceSubscription:(LPPresenceSubscription *)presSub
{
	NSString *title = [NSString stringWithFormat:NSLocalizedString(@"Presence Subscription", @"messages notifications")];
	NSString *descr = nil;
	
	switch ([presSub state]) {
		case LPAuthorizationGranted:
			descr = [NSString stringWithFormat:
				NSLocalizedString(@"%@ was added to your buddy list. You can now see the online status of this contact.", @"messages notifications"),
				[presSub valueForKeyPath:@"contactEntry.address"]];
			break;
			
		case LPAuthorizationRequested:
			descr = [NSString stringWithFormat:
				NSLocalizedString(@"%@ wants to add you as a buddy.", @"messages notifications"),
				[presSub valueForKeyPath:@"contactEntry.address"]];
			break;
			
		case LPAuthorizationLost:
			descr = [NSString stringWithFormat:
				NSLocalizedString(@"Permission to see the online status of contact %@ was lost.", @"messages notifications"),
				[presSub valueForKeyPath:@"contactEntry.address"]];
			break;
	}
	
	[GrowlApplicationBridge notifyWithTitle:title
								description:descr
						   notificationName:LPPresenceSubscriptionReceivedNotificationName
								   iconData:nil
								   priority:1
								   isSticky:NO
							   clickContext:[self p_clickContextForPresenceSubscription:presSub]];
}


#pragma mark GrowlApplicationBridge Delegate


- (NSDictionary *)registrationDictionaryForGrowl
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		
		[NSArray arrayWithObjects:
			LPContactAvailabilityChangedNotificationName,
			LPFirstChatMessageReceivedNotificationName,
			LPChatMessageReceivedNotificationName,
			LPHeadlineMessageReceivedNotificationName,
			LPOfflineMessagesReceivedNotificationName,
			LPPresenceSubscriptionReceivedNotificationName,
			nil],
		GROWL_NOTIFICATIONS_ALL,
		
		[NSArray arrayWithObjects:
			LPContactAvailabilityChangedNotificationName,
			LPFirstChatMessageReceivedNotificationName,
			LPHeadlineMessageReceivedNotificationName,
			LPOfflineMessagesReceivedNotificationName,
			LPPresenceSubscriptionReceivedNotificationName,
			nil],
		GROWL_NOTIFICATIONS_DEFAULT,
		
		nil];
}


- (void)growlNotificationWasClicked:(id)clickContext
{
	NSString *kind = [clickContext objectForKey:@"Kind"];
	
	if ([kind isEqualToString: LPClickContextContactKindValue]) {
		if ([m_delegate respondsToSelector:@selector(notificationsHandler:userDidClickNotificationForContactWithID:)]) {
			unsigned int contactID = [[clickContext objectForKey: LPClickContextContactIDKey] intValue];
			[m_delegate notificationsHandler:self userDidClickNotificationForContactWithID:contactID];
		}
	}
	else if ([kind isEqualToString: LPClickContextHeadlineMessageKindValue]) {
		if ([m_delegate respondsToSelector:@selector(notificationsHandler:userDidClickNotificationForHeadlineMessageWithURI:)]) {
			NSString *messageURI = [clickContext objectForKey: LPClickContextMessageURIKey];
			[m_delegate notificationsHandler:self userDidClickNotificationForHeadlineMessageWithURI:messageURI];
		}
	}
	else if ([kind isEqualToString: LPClickContextOfflineMessagesKindValue]) {
		if ([m_delegate respondsToSelector:@selector(notificationsHandlerUserDidClickNotificationForOfflineMessages:)]) {
			[m_delegate notificationsHandlerUserDidClickNotificationForOfflineMessages:self];
		}
	}
	else if ([kind isEqualToString: LPClickContextPresenceSubscriptionKindValue]) {
		if ([m_delegate respondsToSelector:@selector(notificationsHandlerUserDidClickNotificationForPresenceSubscriptions:)]) {
			[m_delegate notificationsHandlerUserDidClickNotificationForPresenceSubscriptions:self];
		}
	}
}


@end
