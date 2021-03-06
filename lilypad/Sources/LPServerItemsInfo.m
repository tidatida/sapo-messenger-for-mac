//
//  LPServerItemsInfo.m
//  Lilypad
//
//	Copyright (C) 2006-2008 PT.COM,  All rights reserved.
//	Author: Joao Pavao <jpavao@co.sapo.pt>
//
//	For more information on licensing, read the README file.
//	Para mais informações sobre o licenciamento, leia o ficheiro README.
//

#import "LPServerItemsInfo.h"


@implementation LPServerItemsInfo

- (NSString *)p_cacheFilePathname
{
	if ([m_serverHost length] > 0) {
		NSString *supportFolder = LPOurApplicationCachesFolderPath();
		NSString *cacheFilename = [NSString stringWithFormat:@"ServerItemsInfoCache-%@.plist", m_serverHost];
		NSString *cachePathname = [supportFolder stringByAppendingPathComponent:cacheFilename];
		return cachePathname;
	}
	else {
		return nil;
	}
}

- (NSDictionary *)p_dictionaryWithContentsOfCacheFile
{
	// Load the initial disco info dictionary either from the user cache or the app bundle, exactly
	// in this order of preference.
	NSString *cacheFile = [self p_cacheFilePathname];
	
	if (cacheFile == nil || ![[NSFileManager defaultManager] fileExistsAtPath:cacheFile]) {
		cacheFile = [[NSBundle mainBundle] pathForResource:@"ServerItemsInfoCache" ofType:@"plist"];
	}
	
	return [NSDictionary dictionaryWithContentsOfFile:cacheFile];
}

- (void)p_reloadCache
{
	NSDictionary *cacheDict = [self p_dictionaryWithContentsOfCacheFile];
	
	if ([m_serverHost isEqualToString:[cacheDict objectForKey:@"ServerHost"]]) {
		[self willChangeValueForKey:@"serverItems"];
		[m_serverItems release];
		m_serverItems = [[cacheDict objectForKey:@"Items"] copy];
		[self didChangeValueForKey:@"serverItems"];
		
		
		[self willChangeValueForKey:@"identitiesByItem"];
		if ([cacheDict objectForKey:@"ItemsToIdentities"]) {
			[m_serverItemsToIdentities release];
			m_serverItemsToIdentities = [[cacheDict objectForKey:@"ItemsToIdentities"] mutableCopy];
		}
		else {
			[m_serverItemsToIdentities removeAllObjects];
		}
		[self didChangeValueForKey:@"identitiesByItem"];
		
		
		[self willChangeValueForKey:@"featuresByItem"];
		if ([cacheDict objectForKey:@"ItemsToFeatures"]) {
			[m_serverItemsToFeatures release];
			m_serverItemsToFeatures = [[cacheDict objectForKey:@"ItemsToFeatures"] mutableCopy];
		}
		else {
			[m_serverItemsToFeatures removeAllObjects];
		}
		[self didChangeValueForKey:@"featuresByItem"];
		
		
		[self willChangeValueForKey:@"itemsByFeature"];
		if ([cacheDict objectForKey:@"FeaturesToItems"]) {
			[m_featuresToServerItems release];
			m_featuresToServerItems = [[cacheDict objectForKey:@"FeaturesToItems"] mutableCopy];
		}
		else {
			[m_featuresToServerItems removeAllObjects];
		}
		[self didChangeValueForKey:@"itemsByFeature"];
	}
}

- initWithServerHost:(NSString *)host
{
	if (self = [self init]) {
		m_serverHost = [host copy];
		
		NSDictionary *cacheDict = [self p_dictionaryWithContentsOfCacheFile];
		
		if ([m_serverHost isEqualToString:[cacheDict objectForKey:@"ServerHost"]]) {
			m_serverItems = [[cacheDict objectForKey:@"Items"] copy];
			m_serverItemsToIdentities = [[cacheDict objectForKey:@"ItemsToIdentities"] mutableCopy];
			m_serverItemsToFeatures = [[cacheDict objectForKey:@"ItemsToFeatures"] mutableCopy];
			m_featuresToServerItems = [[cacheDict objectForKey:@"FeaturesToItems"] mutableCopy];
		}
		
		if (!m_serverItemsToIdentities)
			m_serverItemsToIdentities = [[NSMutableDictionary alloc] init];
		if (!m_serverItemsToFeatures)
			m_serverItemsToFeatures = [[NSMutableDictionary alloc] init];
		if (!m_featuresToServerItems)
			m_featuresToServerItems = [[NSMutableDictionary alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[m_serverHost release];
	[m_serverItems release];
	[m_serverItemsToIdentities release];
	[m_serverItemsToFeatures release];
	[m_featuresToServerItems release];
	[super dealloc];
}

- (NSArray *)serverItems
{
	return [[m_serverItems retain] autorelease];
}

- (NSDictionary *)identitiesByItem
{
	return [[m_serverItemsToIdentities retain] autorelease];
}

- (NSDictionary *)featuresByItem
{
	return [[m_serverItemsToFeatures retain] autorelease];
}

- (NSDictionary *)itemsByFeature
{
	return [[m_featuresToServerItems retain] autorelease];
}

- (NSArray *)MUCServiceProviderItems
{
	return [m_featuresToServerItems objectForKey:@"http://jabber.org/protocol/muc"];
}

- (void)p_saveCache:(NSTimer *)timer
{
	NSString *cacheFile = [self p_cacheFilePathname];
	
	if (cacheFile != nil) {
		NSDictionary *cacheDict = [NSDictionary dictionaryWithObjectsAndKeys:
			m_serverHost, @"ServerHost",
			m_serverItems, @"Items",
			m_serverItemsToIdentities, @"ItemsToIdentities",
			m_serverItemsToFeatures, @"ItemsToFeatures",
			m_featuresToServerItems, @"FeaturesToItems",
			nil];
		
		[cacheDict writeToFile:cacheFile atomically:YES];
	}
	m_cacheSaveTimerIsRunning = NO;
}

- (void)p_setNeedsCacheSave:(BOOL)flag
{
	if (!m_cacheSaveTimerIsRunning && flag) {
		[NSTimer scheduledTimerWithTimeInterval:20.0
										 target:self
									   selector:@selector(p_saveCache:)
									   userInfo:nil
										repeats:NO];
		m_cacheSaveTimerIsRunning = YES;
	}
}

- (void)handleUpdatedServerHostname:(NSString *)newHostname
{
	[m_serverHost release];
	m_serverHost = [newHostname copy];
	
	[self p_reloadCache];
}

- (void)handleServerItemsUpdated:(NSArray *)items
{
	[self willChangeValueForKey:@"serverItems"];
	[m_serverItems release];
	m_serverItems = [items copy];
	[self didChangeValueForKey:@"serverItems"];
	
	// Clear all the identities
	[self willChangeValueForKey:@"identitiesByItem"];
	[m_serverItemsToIdentities removeAllObjects];
	[self didChangeValueForKey:@"identitiesByItem"];
	
	// Clear the features dictionaries
	[self willChangeValueForKey:@"itemsByFeature"];
	[m_featuresToServerItems removeAllObjects];
	[self didChangeValueForKey:@"itemsByFeature"];
	[self willChangeValueForKey:@"featuresByItem"];
	[m_serverItemsToFeatures removeAllObjects];
	[self didChangeValueForKey:@"featuresByItem"];
	
	
	[self p_setNeedsCacheSave:YES];
}

- (void)handleInfoUpdatedForServerItem:(NSString *)item withName:(NSString *)name identities:(NSArray *)identities features:(NSArray *)features
{
	BOOL isMUCServiceProvider = [features containsObject:@"http://jabber.org/protocol/muc"];
	
	// Keep the identities
	[self willChangeValueForKey:@"identitiesByItem"];
	[m_serverItemsToIdentities setObject:identities forKey:item];
	[self didChangeValueForKey:@"identitiesByItem"];
	
	// Add to the index by item
	[self willChangeValueForKey:@"featuresByItem"];
	[m_serverItemsToFeatures setObject:features forKey:item];
	[self didChangeValueForKey:@"featuresByItem"];
	
	// Add to the index by feature
	[self willChangeValueForKey:@"itemsByFeature"];
	NSEnumerator *featureEnum = [features objectEnumerator];
	NSString *feature;
	while (feature = [featureEnum nextObject]) {
		NSMutableArray *itemsList = [m_featuresToServerItems objectForKey:feature];
		
		if (itemsList == nil) {
			itemsList = [[NSMutableArray alloc] init];
			[m_featuresToServerItems setObject:itemsList forKey:feature];
			[itemsList release];
		}
		
		if (isMUCServiceProvider) [self willChangeValueForKey:@"MUCServiceProviderItems"];
		[itemsList addObject:item];
		if (isMUCServiceProvider) [self didChangeValueForKey:@"MUCServiceProviderItems"];
	}
	[self didChangeValueForKey:@"itemsByFeature"];
	
	[self p_setNeedsCacheSave:YES];
}

@end
