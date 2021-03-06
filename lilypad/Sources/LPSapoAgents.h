//
//  LPSapoAgents.h
//  Lilypad
//
//	Copyright (C) 2006-2008 PT.COM,  All rights reserved.
//	Author: Joao Pavao <jpavao@co.sapo.pt>
//
//	For more information on licensing, read the README file.
//	Para mais informa��es sobre o licenciamento, leia o ficheiro README.
//

#import <Cocoa/Cocoa.h>


@interface LPSapoAgents : NSObject
{
	NSString		*m_serverHost;
	NSDictionary	*m_sapoAgentsDict;
}

- initWithServerHost:(NSString *)host;

- (NSDictionary *)dictionaryRepresentation;
- (NSArray *)rosterContactHostnames;
- (NSArray *)chattingContactHostnames;
- (NSString *)hostnameForService:(NSString *)service;

- (void)handleUpdatedServerHostname:(NSString *)newHostname;
- (void)handleSapoAgentsUpdated:(NSDictionary *)sapoAgents;
@end
