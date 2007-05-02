//
//  LPContactEntry.h
//  Lilypad
//
//	Copyright (C) 2006-2007 PT.COM,  All rights reserved.
//	Author: Joao Pavao <jppavao@criticalsoftware.com>
//
//	For more information on licensing, read the README file.
//	Para mais informa��es sobre o licenciamento, leia o ficheiro README.
//

#import <Cocoa/Cocoa.h>
#import "LPRosterItem.h"
#import "LPCapabilitiesPredicates.h"


@class LPContact;


// Capabilities cache
typedef struct __capabilitiesFlags {
	BOOL	noChatCapFlag:1;
	BOOL	SMSCapFlag:1;
	BOOL	fileTransferCapFlag:1;
} LPEntryCapabilitiesFlags;


@interface LPContactEntry : LPRosterItem <LPCapabilitiesPredicates>
{
	NSString	*m_address;
	NSString	*m_subscription;
	BOOL		m_waitingForAuthorization;
	
	NSImage		*m_avatar;
	NSImage		*m_cachedOfflineAvatar;
	LPStatus	m_status;
	NSString	*m_statusMessage;
	BOOL		m_wasOnlineBeforeDisconnecting;
	
	LPContact	*m_contact;
	
	NSArray				*m_availableResources;	   // NSStrings
	NSMutableDictionary *m_capsFeaturesByResource; // NSString resource -> NSSet capsFeatures
	NSMutableDictionary	*m_resourcesClientInfo;	   // NSString resource -> NSDictionary {clientName, clientVersion, OSName}
	
	// Capabilities cache
	LPEntryCapabilitiesFlags	m_capabilitiesCache;
}

+ entryWithAddress:(NSString *)address;
// Designated initializer
- initWithAddress:(NSString *)address;

- (NSString *)address;
- (NSString *)humanReadableAddress;
- (NSString *)subscription;
- (BOOL)isWaitingForAuthorization;
- (LPContact *)contact;
- (void)moveToContact:(LPContact *)destination;

- (BOOL)hasCustomAvatar;
- (NSImage *)onlineAvatar;
- (NSImage *)offlineAvatar;
- (NSImage *)avatar;
- (NSImage *)framedAvatar;

- (LPStatus)status;
- (NSString *)statusMessage;
- (BOOL)isOnline;
- (BOOL)isInUserRoster;
- (BOOL)isRosterContact;
- (BOOL)presenceShouldBeIgnored;
- (int)multiContactPriority; // smaller means higher priority
- (BOOL)wasOnlineBeforeDisconnecting;

- (LPEntryCapabilitiesFlags)capabilitiesFlags;
- (BOOL)canDoChat;
- (BOOL)canDoSMS;
- (BOOL)canDoFileTransfer;

- (NSArray *)availableResources;

- (BOOL)hasCapsFeature:(NSString *)capsFeature;
- (NSString *)resourceWithCapsFeature:(NSString *)capsFeature;

- (NSString *)allResourcesDescription;
- (NSString *)descriptionForResource:(NSString *)resourceName;

- (void)handleContactEntryChangedWithProperties:(NSDictionary *)properties;
- (void)handleAdditionToContact:(LPContact *)contact;
- (void)handleRemovalFromContact:(LPContact *)contact;

- (void)handlePresenceChangedWithStatus:(LPStatus)status statusMessage:(NSString *)statusMessage;
- (void)handleAvatarChangedWithData:(NSData *)imageData;

- (void)handleAvailableResourcesListChanged:(NSArray *)newResourcesList;
- (void)handleResourcePropertiesChanged:(NSString *)resourceName;
- (void)handleResourceCapabilitiesChanged:(NSString *)resourceName withFeatures:(NSArray *)capsFeatures;
- (void)handleReceivedClientName:(NSString *)clientName clientVersion:(NSString *)clientVersion OSName:(NSString *)OSName forResource:(NSString *)resource;
@end
