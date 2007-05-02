//
//  LPXmlConsoleController.h
//  Lilypad
//
//	Copyright (C) 2006-2007 PT.COM,  All rights reserved.
//	Authors: Joao Pavao <jppavao@criticalsoftware.com>
//           Jason Kim <jason@512k.org>
//
//	For more information on licensing, read the README file.
//	Para mais informa��es sobre o licenciamento, leia o ficheiro README.
//

#import <Cocoa/Cocoa.h>


@class LPAccount;


@interface LPXmlConsoleController : NSWindowController 
{
	IBOutlet NSWindow	*m_inputSheet;
	IBOutlet NSTextView *m_xmlTextView;
	IBOutlet NSTextView *m_inputTextView;
	
	BOOL		m_enabled;
	LPAccount	*m_account;
}

- initWithAccount:(LPAccount *)account;

- (IBAction)clear:(id)sender;
- (IBAction)save:(id)sender;
- (IBAction)showInputSheet:(id)sender;
- (IBAction)inputSheetOK:(id)sender;
- (IBAction)inputSheetCancel:(id)sender;

- (BOOL)isLoggingEnabled;
- (void)setLoggingEnabled:(BOOL)flag;

- (void)appendXmlString:(NSString *)string inbound:(BOOL)isInbound;

@end

