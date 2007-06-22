//
//  LPEditContactController.m
//  Lilypad
//
//	Copyright (C) 2006-2007 PT.COM,  All rights reserved.
//	Author: Joao Pavao <jppavao@criticalsoftware.com>
//
//	For more information on licensing, read the README file.
//	Para mais informações sobre o licenciamento, leia o ficheiro README.
//

#import "LPEditContactController.h"
#import "LPRosterDragAndDrop.h"
#import "LPGroup.h"
#import "LPContact.h"
#import "LPContactEntry.h"
#import "LPRoster.h"
#import "LPAccount.h"
#import "LPUIController.h"
#import "LPRosterController.h"
#import "LPAddContactController.h"
#import "NSString+ConcatAdditions.h"



@interface LPEditContactController (Private)
- (void)p_startObservingEntries:(NSArray *)contactEntries;
- (void)p_stopObservingEntries:(NSArray *)contactEntries;
- (void)p_updateConnectionsDescription;
@end


@implementation LPEditContactController

- initWithContact:(LPContact *)contact delegate:(id)delegate
{
	if (self = [self initWithWindowNibName:@"EditContact"]) {
		m_contact = [contact retain];
		m_delegate = delegate;
		
		[m_contact addObserver:self forKeyPath:@"name" options:0 context:NULL];
		[m_contact addObserver:self forKeyPath:@"online" options:0 context:NULL];
		[m_contact addObserver:self
					forKeyPath:@"contactEntries"
					   options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
					   context:NULL];
		[self p_startObservingEntries:[m_contact contactEntries]];
	}
	return self;
}

- (void)dealloc
{
	[self p_stopObservingEntries:[m_contact contactEntries]];
	[m_contact removeObserver:self forKeyPath:@"contactEntries"];
	[m_contact removeObserver:self forKeyPath:@"online"];
	[m_contact removeObserver:self forKeyPath:@"name"];
	
	[m_contact release];
	[m_addContactController release];
	
	// Retained in -windowDidLoad
	[m_debuggingElements release];
	
	[super dealloc];
}

- (void)windowDidLoad
{
	[m_contactController setContent:[self contact]];
	
	[m_contactNameField setStringValue:[[self contact] name]];
	[self p_updateConnectionsDescription];
	
	// Set up the table view for receiving drops
	[m_contactEntriesTableView registerForDraggedTypes:
		[NSArray arrayWithObjects:LPRosterContactEntryPboardType, LPRosterContactPboardType, nil]];
	
	[m_headerBackground setBackgroundColor:
		[NSColor colorWithPatternImage:( [[self contact] isOnline] ?
										 [NSImage imageNamed:@"chatIDBackground"] :
										 [NSImage imageNamed:@"chatIDBackground_Offline"] )]];
	[m_headerBackground setBorderColor:[NSColor colorWithCalibratedWhite:0.60 alpha:1.0]];
	
	
	// Remove the debugging elements if we don't need them
	// Retain m_debuggingElements before removing them from the window. This way we can still interact normally with its
	// subviews. We'll release it in the end.
	[m_debuggingElements retain];
	if (![[NSUserDefaults standardUserDefaults] boolForKey:@"ShowExtendedInfo"]) {
		[m_contactEntriesTableView removeTableColumn:[m_contactEntriesTableView tableColumnWithIdentifier:@"subs"]];
		[m_contactEntriesTableView removeTableColumn:[m_contactEntriesTableView tableColumnWithIdentifier:@"waiting"]];
		[m_contactEntriesTableView setHeaderView:nil];
		
		[m_debuggingElements removeFromSuperview];
		
		unsigned int savedMask = [m_regularElements autoresizingMask];
		[m_regularElements setAutoresizingMask:( NSViewMaxXMargin | NSViewMinYMargin )];
		[[self window] setContentSize:([m_regularElements visibleRect].size)];
		[m_regularElements setAutoresizingMask:savedMask];
	}
	else {
		// Make the debugging elements view resizable instead of the regular elements view
		[m_regularElements setAutoresizingMask:( NSViewWidthSizable | NSViewMinYMargin )];
		[m_debuggingElements setAutoresizingMask:( NSViewWidthSizable | NSViewHeightSizable )];
	}
}

- (BOOL)validateMenuItem:(id <NSMenuItem>)menuItem
{
	SEL action = [menuItem action];
	BOOL enabled = YES;
	
	if (action == @selector(copy:)) {
		enabled = ([m_entriesController selectionIndex] != NSNotFound);
	}
	
	return enabled;
}

- (void)copy:(id)sender
{
	NSArray			*entries = [m_entriesController selectedObjects];
	NSPasteboard	*pboard = [NSPasteboard generalPasteboard];
	
	[pboard declareTypes:[NSArray arrayWithObjects:NSStringPboardType, nil] owner:nil];
	[pboard setString:[NSString concatenatedStringWithValuesForKey:@"address"
														 ofObjects:entries
												   useDoubleQuotes:NO]
			  forType:NSStringPboardType];
}

- (void)p_startObservingEntries:(NSArray *)contactEntries
{
	NSEnumerator *entriesEnumerator = [contactEntries objectEnumerator];
	LPContactEntry *entry;
	
	while (entry = [entriesEnumerator nextObject]) {
		[entry addObserver:self
				forKeyPath:@"allResourcesDescription"
				   options:0
				   context:NULL];
	}
}

- (void)p_stopObservingEntries:(NSArray *)contactEntries
{
	NSEnumerator *entriesEnumerator = [contactEntries objectEnumerator];
	LPContactEntry *entry;
	
	while (entry = [entriesEnumerator nextObject]) {
		[entry removeObserver:self forKeyPath:@"allResourcesDescription"];
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([keyPath isEqualToString:@"name"]) {
		[m_contactNameField setStringValue:[[self contact] name]];
	}
	else if ([keyPath isEqualToString:@"contactEntries"]) {
		NSArray *oldValue = [change objectForKey:NSKeyValueChangeOldKey];
		NSArray *newValue = [change objectForKey:NSKeyValueChangeNewKey];
		
		NSMutableArray *removedObjs = [oldValue mutableCopy];
		[removedObjs removeObjectsInArray:newValue];
		
		NSMutableArray *addedObjs = [newValue mutableCopy];
		[addedObjs removeObjectsInArray:oldValue];
		
		[self p_stopObservingEntries:removedObjs];
		[self p_startObservingEntries:addedObjs];
		
		if ([[m_contact contactEntries] count] == 0) {
			// The last contact entry was removed.
			[self performSelector:@selector(close) withObject:nil afterDelay:0.0];
		}
		
		[removedObjs release];
		[addedObjs release];
		
		[self p_updateConnectionsDescription];
	}
	else if ([keyPath isEqualToString:@"allResourcesDescription"]) {
		[self p_updateConnectionsDescription];
	}
	else if ([keyPath isEqualToString:@"online"]) {
		[m_headerBackground setBackgroundColor:
			[NSColor colorWithPatternImage:( [[self contact] isOnline] ?
											 [NSImage imageNamed:@"chatIDBackground"] :
											 [NSImage imageNamed:@"chatIDBackground_Offline"] )]];
	}
}

- (LPContact *)contact
{
	return m_contact;
}


- (IBAction)renameContact:(id)sender
{
	NSString	*newName = [sender stringValue];
	
	if (!newName || [newName length] == 0) {
		// No empty names allowed
		NSBeep();
		[m_contactNameField setStringValue:[[self contact] name]];
	}
	else {
		LPRoster	*roster = [[self contact] roster];
		LPContact	*existingContact = [roster contactForName:newName];
		
		if (existingContact == nil) {
			// This name doesn't exist yet in the user's roster
			[[self contact] setName:newName];
		}
		else if (existingContact != [self contact]) {
			// A contact having a name equal to newName already exists
			
			NSString *msg = [NSString stringWithFormat:NSLocalizedString(@"Do you want to merge this contact with the existing "
																		 @"contact named \"%@\"?", @"roster edit warning"),
				newName];
			
			NSString *infoFormatStr = NSLocalizedString(@"A contact named \"%@\" already exists. You may merge the contact being "
														@"renamed with the existing contact. You may also bring up a window to edit "
														@"the existing contact.", @"roster edit warning");
			
			NSAlert *alert = [NSAlert alertWithMessageText:msg
											 defaultButton:NSLocalizedString(@"Edit Existing Contact", @"roster edit warning button")
										   alternateButton:NSLocalizedString(@"Merge Contacts", @"roster edit warning button")
											   otherButton:NSLocalizedString(@"Cancel", @"")
								 informativeTextWithFormat:infoFormatStr, newName];
			
			[alert beginSheetModalForWindow:[self window]
							  modalDelegate:self
							 didEndSelector:@selector(existingContactAlertDidEnd:returnCode:contextInfo:)
								contextInfo:(void *)[existingContact retain]];
			
			// Reset the text field to the original value
			[m_contactNameField setStringValue:[[self contact] name]];
			[m_contactNameField selectText:nil];
		}
	}
}


- (void)existingContactAlertDidEnd:(NSAlert *)alert returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	LPContact *existingContact = [(LPContact *)contextInfo autorelease];
	
	if (returnCode == NSAlertDefaultReturn) {
		if ([m_delegate respondsToSelector:@selector(editContactController:editContact:)]) {
			[m_delegate editContactController:self editContact:existingContact];
		}
	}
	else if (returnCode == NSAlertAlternateReturn) {
		LPContact *myContact = [self contact];
		NSEnumerator *entryEnum = [[myContact contactEntries] objectEnumerator];
		LPContactEntry *entry;
		
		while (entry = [entryEnum nextObject]) {
			[entry moveToContact:existingContact];
		}
		
		if ([m_delegate respondsToSelector:@selector(editContactController:editContact:)]) {
			[m_delegate editContactController:self editContact:existingContact];
		}
	}
}


- (IBAction)addContactEntry:(id)sender
{
	if ([sender menu] == nil) {
		// Create the popup menu
		[sender setMenu:[m_delegate editContactController:self menuForAddingJIDsWithAction:@selector(addContactMenuItemChosen:)]];
	}
	
	[NSMenu popUpContextMenu:[sender menu] withEvent:[NSApp currentEvent] forView:sender];
}

- (IBAction)addContactMenuItemChosen:(id)sender
{
	LPRoster *roster = [[self contact] roster];
	
	if (m_addContactController == nil) {
		m_addContactController = [[LPAddContactController alloc] initWithRoster:roster delegate:self];
	}
	
	[m_addContactController setSapoAgents:[[roster account] sapoAgents]];
	[m_addContactController setHostOfJIDToBeAdded:[sender representedObject]];
	[m_addContactController runForAddingJIDToContact:[self contact] asSheetForWindow:[self window]];
}

- (IBAction)removeContactEntries:(id)sender
{
	NSArray		*selectedEntries = [m_entriesController selectedObjects];
	NSString	*msg;
	
	if ([selectedEntries count] == 1) {
		msg = [NSString stringWithFormat:
			NSLocalizedString(@"Do you really want to remove the address \"%@\" from this contact?", @"warning for roster edits"),
			[[selectedEntries objectAtIndex:0] humanReadableAddress]];
	}
	else {
		msg = [NSString stringWithFormat:
			NSLocalizedString(@"Do you really want to remove the addresses %@ from this contact?", @"warning for roster edits"),
			[NSString concatenatedStringWithValuesForKey:@"humanReadableAddress" ofObjects:selectedEntries useDoubleQuotes:YES]];
	}
	
	NSAlert *alert = [[NSAlert alloc] init];
	
	[alert setMessageText:msg];
	[alert setInformativeText:NSLocalizedString(@"You can't undo this action.", @"")];
	[alert addButtonWithTitle:NSLocalizedString(@"Delete", @"button")];
	[alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"button")];
	
	[alert beginSheetModalForWindow:[self window]
					  modalDelegate:self
					 didEndSelector:@selector(interactiveRemoveJIDsAlertDidEnd:returnCode:contextInfo:)
						contextInfo:(void *)[selectedEntries retain]];
}

- (void)interactiveRemoveJIDsAlertDidEnd:(NSAlert *)alert returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	NSArray *selectedEntries = (NSArray *)contextInfo;
	
	if (returnCode == NSAlertFirstButtonReturn) {
		LPContact	*myContact = [self contact];
		
		NSEnumerator *entryEnumerator = [selectedEntries objectEnumerator];
		LPContactEntry *oneEntry;
		
		while (oneEntry = [entryEnumerator nextObject]) {
			[myContact removeContactEntry:oneEntry];
		}
	}
	
	[selectedEntries release];
	[alert release];
}

- (void)p_updateConnectionsDescription
{
	NSFont *defaultFont = [NSFont userFontOfSize:11];
	NSFont *boldFont = [[NSFontManager sharedFontManager] convertFont:defaultFont toHaveTrait:NSBoldFontMask];
	
	NSDictionary *defaultFontAttribs = [NSDictionary dictionaryWithObject:defaultFont forKey:NSFontAttributeName];
	NSDictionary *boldFontAttribs = [NSDictionary dictionaryWithObject:boldFont forKey:NSFontAttributeName];
	
	NSTextStorage *ts = [m_connectionsDescriptionView textStorage];
	[ts beginEditing];
	[ts deleteCharactersInRange:NSMakeRange(0, [ts length])];
	
	NSEnumerator *contactEntryEnumerator = [[[self contact] contactEntries] objectEnumerator];
	LPContactEntry *contactEntry;
	
	while (contactEntry = [contactEntryEnumerator nextObject]) {
		NSEnumerator *resourceEnumerator = [[contactEntry availableResources] objectEnumerator];
		NSString *resourceName;
		
		while (resourceName = [resourceEnumerator nextObject]) {
			// Output a header in bold
			NSString *headerStr = [NSString stringWithFormat:@"%@ (%@):\n", [contactEntry address], resourceName];
			NSAttributedString *headerAttrStr = [[NSAttributedString alloc] initWithString:headerStr attributes:boldFontAttribs];
			[ts appendAttributedString:headerAttrStr];
			[headerAttrStr release];
			
			// Output the resource properties
			NSString *resourcePropsStr = [contactEntry descriptionForResource:resourceName];
			NSAttributedString *resourcePropsAttribStr = [[NSAttributedString alloc] initWithString:resourcePropsStr
																						 attributes:defaultFontAttribs];
			[ts appendAttributedString:resourcePropsAttribStr];
			[resourcePropsAttribStr release];
			
			[ts replaceCharactersInRange:NSMakeRange([ts length], 0) withString:@"\n"];
		}
	}
	
	[ts endEditing];
}


#pragma mark -
#pragma mark LPAddContactController Delegate


- (NSMenu *)addContactController:(LPAddContactController *)addContactCtrl menuForAddingJIDsWithAction:(SEL)action
{
	return [m_delegate editContactController:self menuForAddingJIDsWithAction:action];
}


#pragma mark -
#pragma mark NSTableView Delegate / Data Source Methods


- (BOOL)tableView:(NSTableView *)aTableView writeRows:(NSArray *)rows toPasteboard:(NSPasteboard *)pboard
{
	// This method is deprecated in 10.4, but the alternative doesn't exist on 10.3, so we have to use this one.
	
	NSMutableArray	*draggedEntriesList = [NSMutableArray arrayWithCapacity:[rows count]];
	NSEnumerator	*rowNrEnum = [rows objectEnumerator];
	NSNumber		*rowNr;
	
	id				entriesList = [m_entriesController arrangedObjects];
	
	while (rowNr = [rowNrEnum nextObject]) {
		id entry = [entriesList objectAtIndex:[rowNr unsignedIntValue]];
		[draggedEntriesList addObject:entry];
	}
	
	LPAddContactEntriesToPasteboard(pboard, draggedEntriesList);
	
	return YES;
}


- (NSDragOperation)tableView:(NSTableView *)aTableView validateDrop:(id <NSDraggingInfo>)info proposedRow:(int)row proposedDropOperation:(NSTableViewDropOperation)operation
{
	NSDragOperation		resultOp = NSDragOperationNone;
	NSArray				*draggedTypes = [[info draggingPasteboard] types];
	
	if ([draggedTypes containsObject:LPRosterContactEntryPboardType]) {
		NSArray *entriesBeingDragged = LPRosterContactEntriesBeingDragged(info);
		
		if ([[entriesBeingDragged valueForKey:@"contact"] containsObject:[self contact]])
			resultOp = NSDragOperationNone;
		else
			resultOp = NSDragOperationGeneric;
	}
	else if ([draggedTypes containsObject:LPRosterContactPboardType]) {
		NSArray *contactsBeingDragged = LPRosterContactsBeingDragged(info);
		
		if ([contactsBeingDragged containsObject:[self contact]])
			resultOp = NSDragOperationNone;
		else
			resultOp = NSDragOperationGeneric;
	}
	
	if (resultOp == NSDragOperationGeneric) {
		// Highlight the whole table
		[aTableView setDropRow:-1 dropOperation:NSTableViewDropOn];
	}
	
	return resultOp;
}


- (BOOL)tableView:(NSTableView *)aTableView acceptDrop:(id <NSDraggingInfo>)info row:(int)row dropOperation:(NSTableViewDropOperation)operation
{
	NSPasteboard		*pboard = [info draggingPasteboard];
	NSArray				*draggedTypes = [pboard types];
	NSDragOperation		dragOpMask = [info draggingSourceOperationMask];
	
	if ([draggedTypes containsObject:LPRosterContactEntryPboardType]) {
		NSArray			*entriesBeingDragged = LPRosterContactEntriesBeingDragged(info);
		NSEnumerator	*entriesEnum = [entriesBeingDragged objectEnumerator];
		LPContactEntry	*entry;
		
		while (entry = [entriesEnum nextObject]) {
			if (dragOpMask & NSDragOperationGeneric) {
				if (![[[self contact] contactEntries] containsObject:entry]) {
					[entry moveToContact:[self contact]];
				}
			}
		}
	}
	else if ([draggedTypes containsObject:LPRosterContactPboardType]) {
		NSArray			*contactsBeingDragged = LPRosterContactsBeingDragged(info);
		NSEnumerator	*contactsEnum = [contactsBeingDragged objectEnumerator];
		LPContact		*contact;
		
		while (contact = [contactsEnum nextObject]) {
			NSEnumerator	*entriesEnum = [[contact contactEntries] objectEnumerator];
			LPContactEntry	*entry;
			
			while (entry = [entriesEnum nextObject]) {
				if (dragOpMask & NSDragOperationGeneric) {
					if (![[[self contact] contactEntries] containsObject:entry]) {
						[entry moveToContact:[self contact]];
					}
				}
			}
		}
	}
	
    return YES;
}



#pragma mark -
#pragma mark NSWindow Delegate Methods


- (void)windowWillClose:(NSNotification *)aNotification
{
	if ([m_delegate respondsToSelector:@selector(editContactControllerWindowWillClose:)]) {
		[m_delegate editContactControllerWindowWillClose:self];
	}
}


@end
