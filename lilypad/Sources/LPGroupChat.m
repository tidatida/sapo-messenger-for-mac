//
//  LPGroupChat.m
//  Lilypad
//
//	Copyright (C) 2006-2008 PT.COM,  All rights reserved.
//	Author: Joao Pavao <jpavao@co.sapo.pt>
//
//	For more information on licensing, read the README file.
//	Para mais informações sobre o licenciamento, leia o ficheiro README.
//

#import "LPGroupChat.h"
#import "LPAccount.h"
#import "LPGroupChatContact.h"

#warning Estes devem dar para apagar depois de tratar dos outros warnings
#import "LPAccountsController.h"
#import "LPAccount.h"
#import "LPChatsManager.h"


#define NSStringWithFormatIfNotEmpty(formatStr, argStr)	\
	([argStr length] > 0 ? [NSString stringWithFormat:formatStr, argStr] : @"")


@interface LPGroupChat ()  // Private Methods
- (void)p_setActive:(BOOL)flag;
- (void)p_setNickname:(NSString *)nickname;
- (void)p_setLastSetNickname:(NSString *)nickname;
- (void)p_setLastUsedPassword:(NSString *)password;
- (void)p_addParticipant:(LPGroupChatContact *)contact;
- (void)p_removeParticipant:(LPGroupChatContact *)contact;
- (LPGroupChatContact *)p_participantWithNickname:(NSString *)nickname;
- (void)p_updateParticipantNicknameFrom:(NSString *)oldNickname to:(NSString *)newNickname;
- (void)p_doEmitUserSystemMessages;
@end


@implementation LPGroupChat

+ (BOOL)automaticallyNotifiesObserversForKey:(NSString *)key
{
	if ([key isEqualToString:@"nickname"] || [key isEqualToString:@"topic"]) {
		// Avoid triggering change notifications on calls to -[LPGroupChat setNickname:] or -[LPGroupChat setTopic:]
		return NO;
	} else {
		return YES;
	}
}

+ groupChatForRoomWithJID:(NSString *)roomJID onAccount:(LPAccount *)account groupChatID:(int)ID nickname:(NSString *)nickname password:(NSString *)password
{
	return [[[[self class] alloc] initForRoomWithJID:roomJID onAccount:account groupChatID:ID nickname:nickname password:password] autorelease];
}

- initForRoomWithJID:(NSString *)roomJID onAccount:(LPAccount *)account groupChatID:(int)ID nickname:(NSString *)nickname password:(NSString *)password
{
	if (self = [super init]) {
		m_ID = ID;
		m_account = [account retain];
		m_roomJID = [roomJID copy];
		m_nickname = [nickname copy];
		m_lastSetNickname = [nickname copy];
		m_lastUsedPassword = [password copy];
		
		m_participants = [[NSMutableSet alloc] init];
		m_participantsByNickname = [[NSMutableDictionary alloc] init];
		
		[m_account addObserver:self
					forKeyPath:@"online"
					   options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
					   context:NULL];
	}
	return self;
}

- (void)dealloc
{
	[m_account removeObserver:self forKeyPath:@"online"];
	
	[m_account release];
	[m_roomJID release];
	[m_nickname release];
	[m_lastSetNickname release];
	[m_lastUsedPassword release];
	[m_topic release];
	[m_participants release];
	[m_participantsByNickname release];
	[m_pendingInvites release];
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

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([keyPath isEqualToString:@"online"]) {
		BOOL wasOnline = [[change objectForKey:NSKeyValueChangeOldKey] boolValue];
		BOOL isOnline  = [[change objectForKey:NSKeyValueChangeNewKey] boolValue];
		
		if (wasOnline && !isOnline) {
			// Account went down
			if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
				NSString *sysMsg = [NSString stringWithFormat:
									NSLocalizedString(@"The account \"%@\", which is used by this group-chat, has been"
													  @" disconnected.",
													  @"Chat room system message"),
									[object description]];
				
				[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
			}
			[self handleDidLeaveGroupChat];
		}
		else if (!wasOnline && isOnline) {
			// Account went up
			if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
				NSString *sysMsg = [NSString stringWithFormat:
									NSLocalizedString(@"The account \"%@\", which is used by this group-chat, has been"
													  @" reconnected and is now back online.",
													  @"Chat room system message"),
									[object description]];
				
				[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
			}
			[self retryJoinWithNickname:[self lastSetNickname] password:[self lastUsedPassword]];
		}
	}
	else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)retryJoinWithNickname:(NSString *)nickname password:(NSString *)password
{
	NSAssert( ![self isActive] , @"retryJoinWithPassword: shouldn't be invoked because we have already successfully joined the room!");
	
	if (![[self nickname] isEqualToString:nickname]) {
		[self p_setNickname:nickname];
	}
	if (![[self lastSetNickname] isEqualToString:nickname]) {
		[self p_setLastSetNickname:nickname];
	}
	if (![[self lastUsedPassword] isEqualToString:password]) {
		[self p_setLastUsedPassword:password];
	}
	
	[LFAppController groupChatRetryJoin:[self ID] nickname:nickname password:password];
}

- (int)ID
{
	return m_ID;
}

- (LPAccount *)account
{
	return [[m_account retain] autorelease];
}

- (NSString *)roomJID
{
	return [[m_roomJID copy] autorelease];
}

- (NSString *)roomName
{
	return [m_roomJID JIDUsernameComponent];
}

- (NSString *)nickname
{
	return [[m_nickname copy] autorelease];
}

- (void)setNickname:(NSString *)newNick
{
	if (![m_lastSetNickname isEqualToString:newNick]) {
		[self p_setLastSetNickname:newNick];
	}
	
	[LFAppController groupChatSetNicknameOnRoom:[self ID] to:newNick];
}

- (NSString *)lastSetNickname
{
	return [[m_lastSetNickname copy] autorelease];
}

- (NSString *)lastUsedPassword
{
	return [[m_lastUsedPassword copy] autorelease];
}

- (BOOL)isActive
{
	return m_isActive;
}

- (NSString *)topic
{
	return [[m_topic copy] autorelease];
}

- (void)setTopic:(NSString *)newTopic
{
	[LFAppController groupChatSetTopicOnRoom:[self ID] to:newTopic];
}

- (void)inviteJID:(NSString *)jid withReason:(NSString *)reason
{
	if ([self isActive]) {
		[LFAppController groupChatInvite:jid
									room:[self roomJID]
							 accountUUID:[[self account] UUID]
								  reason:reason];
		
		if ([m_delegate respondsToSelector:@selector(groupChat:didInviteJID:withReason:)]) {
			[m_delegate groupChat:self didInviteJID:jid withReason:reason];
		}
	}
	else {
		if (m_pendingInvites == nil)
			m_pendingInvites = [[NSMutableArray alloc] init];
		
		// Store it in the list of pending invitations
		[m_pendingInvites addObject:[NSDictionary dictionaryWithObjectsAndKeys:
			jid, @"JID", reason, @"Reason", nil]];
	}
}

- (LPGroupChatContact *)myGroupChatContact
{
	return [[m_myGroupChatContact retain] autorelease];
}

- (NSSet *)participants
{
	return [[m_participants retain] autorelease];
}

- (void)reloadRoomConfigurationForm
{
	[LFAppController fetchGroupChatConfigurationForm:[self ID]];
}

- (void)submitRoomConfigurationForm:(NSString *)configurationXMLForm
{
	[LFAppController submitGroupChatConfigurationForm:[self ID] :configurationXMLForm];
}

- (void)sendPlainTextMessage:(NSString *)message
{
	[LFAppController groupChatMessageSend:[self ID] plain:message];
}

- (void)endGroupChat
{
	[[LPChatsManager chatsManager] endGroupChat:self];
}

#pragma mark -

- (void)p_setActive:(BOOL)flag
{
	if (flag != m_isActive) {
		[self willChangeValueForKey:@"active"];
		m_isActive = flag;
		[self didChangeValueForKey:@"active"];
	}
}

- (void)p_setNickname:(NSString *)nickname
{
	if (m_nickname != nickname) {
		[self willChangeValueForKey:@"nickname"];
		[m_nickname release];
		m_nickname = [nickname copy];
		[self didChangeValueForKey:@"nickname"];
	}
}

- (void)p_setLastSetNickname:(NSString *)nickname
{
	if (m_lastSetNickname != nickname) {
		[self willChangeValueForKey:@"lastSetNickname"];
		[m_lastSetNickname release];
		m_lastSetNickname = [nickname copy];
		[self didChangeValueForKey:@"lastSetNickname"];
	}
}

- (void)p_setLastUsedPassword:(NSString *)password
{
	if (m_lastUsedPassword != password) {
		[self willChangeValueForKey:@"lastUsedPassword"];
		[m_lastUsedPassword release];
		m_lastUsedPassword = [password copy];
		[self didChangeValueForKey:@"lastUsedPassword"];
	}
}

- (void)p_addParticipant:(LPGroupChatContact *)contact
{
	NSSet *changeSet = [NSSet setWithObject:contact];
	
	[self willChangeValueForKey:@"participants" withSetMutation:NSKeyValueUnionSetMutation usingObjects:changeSet];
	[m_participants addObject:contact];
	[self didChangeValueForKey:@"participants" withSetMutation:NSKeyValueUnionSetMutation usingObjects:changeSet];
	
	[m_participantsByNickname setObject:contact forKey:[contact nickname]];
}

- (void)p_removeParticipant:(LPGroupChatContact *)contact
{
	NSSet *changeSet = [NSSet setWithObject:contact];
	
	[self willChangeValueForKey:@"participants" withSetMutation:NSKeyValueMinusSetMutation usingObjects:changeSet];
	[m_participants removeObject:contact];
	[self didChangeValueForKey:@"participants" withSetMutation:NSKeyValueMinusSetMutation usingObjects:changeSet];
	
	[m_participantsByNickname removeObjectForKey:[contact nickname]];
	
	if (contact == m_myGroupChatContact) {
		[self willChangeValueForKey:@"myGroupChatContact"];
		m_myGroupChatContact = nil;
		[self didChangeValueForKey:@"myGroupChatContact"];
	}
}

- (LPGroupChatContact *)p_participantWithNickname:(NSString *)nickname
{
	return [m_participantsByNickname objectForKey:nickname];
}

- (void)p_updateParticipantNicknameFrom:(NSString *)oldNickname to:(NSString *)newNickname
{
	LPGroupChatContact *contact = [m_participantsByNickname objectForKey:oldNickname];
	
	[contact retain];
	[m_participantsByNickname removeObjectForKey:oldNickname];
	[contact handleChangedNickname:newNickname];
	[m_participantsByNickname setObject:contact forKey:newNickname];
	[contact release];
}

- (void)p_doEmitUserSystemMessages
{
	m_emitUserSystemMessages = YES;
}


#pragma mark -

- (void)handleDidJoinGroupChatWithJID:(NSString *)roomJID nickname:(NSString *)nickname
{
	[self p_setActive:YES];
	
	if (![[self nickname] isEqualToString:nickname]) {
		[self p_setNickname:nickname];
	}
	
	[self performSelector:@selector(p_doEmitUserSystemMessages) withObject:nil afterDelay:5.0];
	
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
		[m_delegate groupChat:self didReceiveSystemMessage:NSLocalizedString(@"Chat-room was joined successfully.",
																			 @"Chat room system message")];
	}
	
	// Are there any pending invitations waiting to be sent?
	NSEnumerator *inviteEnum = [m_pendingInvites objectEnumerator];
	NSDictionary *inviteDict;
	while (inviteDict = [inviteEnum nextObject]) {
		[self inviteJID:[inviteDict objectForKey:@"JID"] withReason:[inviteDict objectForKey:@"Reason"]];
	}
	[m_pendingInvites release]; m_pendingInvites = nil;
}

- (void)handleDidLeaveGroupChat
{
	[self p_setActive:NO];
	
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
		[m_delegate groupChat:self didReceiveSystemMessage:NSLocalizedString(@"You have left the chat-room.",
																			 @"Chat room system message")];
	}
	
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(p_doEmitUserSystemMessages) object:nil];
}

- (void)handleDidCreateGroupChat
{
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
		[m_delegate groupChat:self didReceiveSystemMessage:NSLocalizedString(@"Chat-room was created.",
																			 @"Chat room system message")];
	}
}

- (void)handleDidDestroyGroupChatWithReason:(NSString *)reason alternateRoomJID:(NSString *)alternateRoomJID
{
#warning This notification could be handled in a more user-friendly way. Simply showing a chat-room JID to the user is lame!
	
	NSString *sysMsg = ( ([alternateRoomJID length] > 0) ?
						 [NSString stringWithFormat:
							 NSLocalizedString(@"Chat-room was destroyed. Please join the alternative chat-room at \"%@\".",
											   @"Chat room system message"), alternateRoomJID] :
						 NSLocalizedString(@"Chat-room was destroyed.", @"Chat room system message") );
	
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
		[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
	}
}

- (void)handleContactDidJoinGroupChatWithNickname:(NSString *)nickname JID:(NSString *)jid role:(NSString *)role affiliation:(NSString *)affiliation
{
	LPGroupChatContact *newContact = [LPGroupChatContact groupChatContactWithNickame:nickname realJID:jid
																				role:role affiliation:affiliation
																		   groupChat:self];
	[self p_addParticipant:newContact];
	
	if ([m_nickname isEqualToString:nickname]) {
		[self willChangeValueForKey:@"myGroupChatContact"];
		m_myGroupChatContact = newContact;
		[self didChangeValueForKey:@"myGroupChatContact"];
	}
	
	
	if (m_emitUserSystemMessages) {
		// Send a system message to our delegate
		NSString *sysMsg = [NSString stringWithFormat:
			NSLocalizedString(@"\"%@\"%@ has joined the chat. <%@, %@>", @"Chat room system message"),
			[newContact userPresentableNickname],
			NSStringWithFormatIfNotEmpty(@" (%@)", jid),
			role, affiliation];
		
		if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
			[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
		}
	}
}

- (void)handleContactWithNickname:(NSString *)nickname didChangeRoleTo:(NSString *)role affiliationTo:(NSString *)affiliation
{
	LPGroupChatContact *contact = [self p_participantWithNickname:nickname];
	NSString *jid = [contact realJID];
	
	[contact handleChangedRole:role orAffiliation:affiliation];
	
	if (m_emitUserSystemMessages) {
		// Send a system message to our delegate
		NSString *sysMsg = [NSString stringWithFormat:
			NSLocalizedString(@"\"%@\"%@ is now <%@, %@>.", @"Chat room system message"),
			[contact userPresentableNickname],
			NSStringWithFormatIfNotEmpty(@" (%@)", jid),
			role, affiliation];
		
		if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
			[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
		}
	}
}

- (void)handleContactWithNickname:(NSString *)nickname didChangeStatusTo:(LPStatus)status statusMessageTo:(NSString *)statusMsg
{
	[[self p_participantWithNickname:nickname] handleChangedStatus:status statusMessage:statusMsg];
}

- (void)handleContactWithNickname:(NSString *)nickname didChangeNicknameFrom:(NSString *)old_nickname to:(NSString *)new_nickname
{
	NSString *oldPresentableNickname = [[self p_participantWithNickname:old_nickname] userPresentableNickname];
	[self p_updateParticipantNicknameFrom:old_nickname to:new_nickname];
	NSString *newPresentableNickname = [[self p_participantWithNickname:new_nickname] userPresentableNickname];
	
	if ([m_nickname isEqualToString:nickname]) {
		[self p_setNickname:new_nickname];
	}
	
	// Send a system message to our delegate
	NSString *sysMsg = [NSString stringWithFormat:
		NSLocalizedString(@"\"%@\" is now known as \"%@\".", @"Chat room system message"),
		oldPresentableNickname, newPresentableNickname];
	
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
		[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
	}
}

- (void)handleContactWithNickname:(NSString *)nickname wasKickedBy:(NSString *)actor reason:(NSString *)reason
{
	LPGroupChatContact *contact = [self p_participantWithNickname:nickname];
	NSString *userPresentableNickname = [contact userPresentableNickname];
	NSString *jid = [contact realJID];
	
	[self p_removeParticipant:contact];
	
	// Send a system message to our delegate
	NSString *sysMsg = [NSString stringWithFormat:
		NSLocalizedString(@"\"%@\"%@ was kicked%@%@.", @"Chat room system message"),
		userPresentableNickname,
		NSStringWithFormatIfNotEmpty(@" (%@)", jid),
		NSStringWithFormatIfNotEmpty(@" by \"%@\"", actor),
		NSStringWithFormatIfNotEmpty(@" (reason: %@)", reason)];
	
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
		[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
	}
	
	// Are we the one being kicked?
	if ([m_nickname isEqualToString:nickname]) {
		if ([m_delegate respondsToSelector:@selector(groupChat:didGetKickedBy:reason:)]) {
			[m_delegate groupChat:self didGetKickedBy:[self p_participantWithNickname:actor] reason:reason];
		}
	}
}

- (void)handleContactWithNickname:(NSString *)nickname wasBannedBy:(NSString *)actor reason:(NSString *)reason
{
	LPGroupChatContact *contact = [self p_participantWithNickname:nickname];
	NSString *userPresentableNickname = [contact userPresentableNickname];
	NSString *jid = [contact realJID];
	
	[self p_removeParticipant:contact];
	
	// Send a system message to our delegate
	NSString *sysMsg = [NSString stringWithFormat:
		NSLocalizedString(@"\"%@\"%@ was banned%@%@.", @"Chat room system message"),
		userPresentableNickname,
		NSStringWithFormatIfNotEmpty(@" (%@)", jid),
		NSStringWithFormatIfNotEmpty(@" by \"%@\"", actor),
		NSStringWithFormatIfNotEmpty(@" (reason: %@)", reason)];
	
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
		[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
	}
	
	// Are we the one being banned?
	if ([m_nickname isEqualToString:nickname]) {
		if ([m_delegate respondsToSelector:@selector(groupChat:didGetBannedBy:reason:)]) {
			[m_delegate groupChat:self didGetBannedBy:[self p_participantWithNickname:actor] reason:reason];
		}
	}
}

- (void)handleContactWithNickname:(NSString *)nickname wasRemovedFromChatBy:(NSString *)actor reason:(NSString *)reason dueTo:(NSString *)dueTo
{
	LPGroupChatContact *contact = [self p_participantWithNickname:nickname];
	NSString *userPresentableNickname = [contact userPresentableNickname];
	LPGroupChatContact *actorContact = [self p_participantWithNickname:actor];
	NSString *actorPresentableNickname = [actorContact userPresentableNickname];
	NSString *jid = [contact realJID];
	
	[self p_removeParticipant:contact];
	
	// Send a system message to our delegate
	NSString *sysMsg = [NSString stringWithFormat:
		NSLocalizedString(@"\"%@\"%@ was removed from the room%@ (due to: \"%@\"%@).", @"Chat room system message"),
		userPresentableNickname,
		NSStringWithFormatIfNotEmpty(@" (%@)", jid),
		NSStringWithFormatIfNotEmpty(@" by \"%@\"", actorPresentableNickname),
		dueTo,
		NSStringWithFormatIfNotEmpty(@", reason: %@", reason)];
	
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
		[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
	}
	
	
	if ([m_nickname isEqualToString:nickname])
		; // Do something different if we're the one being removed?
}

- (void)handleContactWithNickname:(NSString *)nickname didLeaveWithStatusMessage:(NSString *)status
{
	LPGroupChatContact *contact = [self p_participantWithNickname:nickname];
	NSString *userPresentableNickname = [contact userPresentableNickname];
	NSString *jid = [contact realJID];
	
	[self p_removeParticipant:contact];
	
	if (m_emitUserSystemMessages) {
		// Send a system message to our delegate
		NSString *sysMsg = [NSString stringWithFormat:
			NSLocalizedString(@"\"%@\"%@ has left the room%@.", @"Chat room system message"),
			userPresentableNickname,
			NSStringWithFormatIfNotEmpty(@" (%@)", jid),
			NSStringWithFormatIfNotEmpty(@" (%@)", status)];
		
		if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
			[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
		}
	}
	
	
	if ([m_nickname isEqualToString:nickname])
		; // Do something different if we're the one leaving?
}

- (void)handleGroupChatErrorWithCode:(int)code message:(NSString *)msg
{
	// Send a system message to our delegate
	NSString *sysMsg = [NSString stringWithFormat:
		NSLocalizedString(@"Chat room error: %@ (%d)", @"Chat room system message"),
		msg, code];
	
	
	if (code == 401) {
		// Password required or wrong password was provided
		if ([m_delegate respondsToSelector:@selector(groupChat:unableToProceedDueToWrongPasswordWithErrorMessage:)]) {
			[m_delegate groupChat:self unableToProceedDueToWrongPasswordWithErrorMessage:sysMsg];
		}
	}
	else if (code == 409) {
		// Nickname is already taken
		if ([m_delegate respondsToSelector:@selector(groupChat:unableToProceedDueToNicknameAlreadyInUseWithErrorMessage:)]) {
			[m_delegate groupChat:self unableToProceedDueToNicknameAlreadyInUseWithErrorMessage:sysMsg];
		}
	}
	else {
		// Some other error
		if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
			[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
		}
	}
}

- (void)handleTopicChangedTo:(NSString *)newTopic by:(NSString *)actor
{
	[self willChangeValueForKey:@"topic"];
	[m_topic release];
	m_topic = [newTopic copy];
	[self didChangeValueForKey:@"topic"];
	
	// Send a system message to our delegate
	LPGroupChatContact *contact = [self p_participantWithNickname:actor];
	NSString *userPresentableNickname = [contact userPresentableNickname];
	NSString *jid = [contact realJID];
	
	NSString *sysMsg;
	
	if ([actor length] > 0) {
		sysMsg = [NSString stringWithFormat:
			NSLocalizedString(@"\"%@\"%@ has changed the topic to: \"%@\"", @"Chat room system message"),
			userPresentableNickname,
			NSStringWithFormatIfNotEmpty(@" (%@)", jid),
			newTopic];
	}
	else {
		sysMsg = [NSString stringWithFormat:
			NSLocalizedString(@"The topic has been set to: \"%@\"", @"Chat room system message"),
			newTopic];
	}
	
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveSystemMessage:)]) {
		[m_delegate groupChat:self didReceiveSystemMessage:sysMsg];
	}
}

- (void)handleReceivedMessageFromNickname:(NSString *)nickname plainBody:(NSString *)plainBody
{
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveMessage:fromContact:)]) {
		LPGroupChatContact *contact = [m_participantsByNickname objectForKey:nickname];
		[m_delegate groupChat:self didReceiveMessage:plainBody fromContact:contact];
	}
}

- (void)handleReceivedConfigurationForm:(NSString *)configForm errorMessage:(NSString *)errorMsg
{
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveRoomConfigurationForm:errorMessage:)]) {
		[m_delegate groupChat:self didReceiveRoomConfigurationForm:configForm errorMessage:errorMsg];
	}

}

- (void)handleResultOfConfigurationModification:(BOOL)succeeded errorMessage:(NSString *)errorMsg
{
	if ([m_delegate respondsToSelector:@selector(groupChat:didReceiveResultOfRoomConfigurationModification:errorMessage:)]) {
		[m_delegate groupChat:self didReceiveResultOfRoomConfigurationModification:succeeded errorMessage:errorMsg];
	}
	
}

@end
