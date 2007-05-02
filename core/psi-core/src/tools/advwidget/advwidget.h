/*
 * advwidget.h - AdvancedWidget template class
 * Copyright (C) 2005  Michail Pishchagin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef ADVWIDGET_H
#define ADVWIDGET_H

#include <qwidget.h>

class GAdvancedWidget : public QObject
{
	Q_OBJECT
public:
	GAdvancedWidget(QWidget *parent);

	static bool stickEnabled();
	static void setStickEnabled(bool val);

	static int stickAt();
	static void setStickAt(int val);

	static bool stickToWindows();
	static void setStickToWindows(bool val);

	void restoreSavedGeometry(QRect savedGeometry);
	void doFlash(bool on);

#ifdef Q_OS_WIN
	bool winEvent(MSG *msg);
#endif

	void moveEvent(QMoveEvent *e);

	void preSetCaption();
	void postSetCaption();

	void windowActivationChange(bool oldstate);

public:
	class Private;
private:
	Private *d;
};

template <class BaseClass>
class AdvancedWidget : public BaseClass
{
private:
	GAdvancedWidget *gAdvWidget;

public:
	AdvancedWidget(QWidget *parent = 0, Qt::WindowFlags f = 0)
		: BaseClass(parent)
	{
		BaseClass::setWindowFlags(f);
		gAdvWidget = new GAdvancedWidget( this );
	}

	static bool stickEnabled() { return GAdvancedWidget::stickEnabled(); }
	static void setStickEnabled( bool val ) { GAdvancedWidget::setStickEnabled( val ); }

	static int stickAt() { return GAdvancedWidget::stickAt(); }
	static void setStickAt( int val ) { GAdvancedWidget::setStickAt( val ); }

	static bool stickToWindows() { return GAdvancedWidget::stickToWindows(); }
	static void setStickToWindows( bool val ) { GAdvancedWidget::setStickToWindows( val ); }

	QRect saveableGeometry() const
	{
		return QRect(BaseClass::pos(), BaseClass::size());
	}

	virtual void restoreSavedGeometry(QRect savedGeometry)
	{
		gAdvWidget->restoreSavedGeometry(savedGeometry);
	}

	void doFlash( bool on )
	{
		gAdvWidget->doFlash( on );
	}

#ifdef Q_OS_WIN
	bool winEvent( MSG *msg )
	{
		return gAdvWidget->winEvent( msg );
	}
#endif

	void moveEvent( QMoveEvent *e )
	{
		gAdvWidget->moveEvent(e);
	}

	void setWindowTitle( const QString &c )
	{
		gAdvWidget->preSetCaption();
		BaseClass::setWindowTitle( c );
		gAdvWidget->postSetCaption();
	}

protected:
	void windowActivationChange( bool oldstate )
	{
		gAdvWidget->windowActivationChange( oldstate );
		BaseClass::windowActivationChange( oldstate );
	}
};

#endif
