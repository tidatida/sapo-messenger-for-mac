//
//  NSString+JIDAdditions.h
//  Lilypad
//
//	Copyright (C) 2006-2007 PT.COM,  All rights reserved.
//	Author: Joao Pavao <jppavao@criticalsoftware.com>
//
//	For more information on licensing, read the README file.
//	Para mais informa��es sobre o licenciamento, leia o ficheiro README.
//

#import <Cocoa/Cocoa.h>


@interface NSString (JIDAdditions)
- (NSString *)bareJIDComponent;
- (NSString *)JIDResourceNameComponent;
- (NSString *)JIDUsernameComponent;
- (NSString *)JIDHostnameComponent;

- (BOOL)isPhoneJID;
- (NSString *)userPresentablePhoneNrRepresentation;
- (NSString *)internalPhoneNrRepresentation;
- (NSString *)internalPhoneJIDRepresentation;

- (NSString *)userPresentableJIDAsPerAgentsDictionary:(NSDictionary *)sapoAgentsDict;

@end
