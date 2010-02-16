/**
 * @file global.cpp
 * @author Wei-Cheng Pan
 *
 * KomiX, a comics viewer.
 * Copyright (C) 2008  Wei-Cheng Pan <legnaleurc@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "global.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QTranslator>
#include <QtGlobal>
#include <QtGui/QImageReader>

#include <algorithm>

namespace {

	inline void tl( QByteArray & s ) {
		s = s.toLower();
	}

	inline QStringList uniqueList() {
		Q_ASSERT( QCoreApplication::instance() != NULL );
		std::list< QByteArray > uniList = QImageReader::supportedImageFormats().toStdList();

		std::for_each( uniList.begin(), uniList.end(), tl );
		uniList.sort();
		uniList.unique();

		QStringList result;

		std::copy( uniList.begin(), uniList.end(), std::back_inserter( result ) );
		return result;
	}

	std::list< KomiX::FileMenuHook > & fileMenuHooks() {
		static std::list< KomiX::FileMenuHook > hooks;
		return hooks;
	}

	KomiX::TranslationMap & translations() {
		static KomiX::TranslationMap t;
		return t;
	}

}

namespace KomiX {

	const std::list< FileMenuHook > & getFileMenuHooks() {
		return fileMenuHooks();
	}

	bool registerFileMenuHook( FileMenuHook hook ) {
		fileMenuHooks().push_back( hook );
		return true;
	}

	void loadTranslations() {
		QDir d( qApp->applicationDirPath() );
#ifdef _MSC_VER
		d.cd( ".." );
#endif
		QList< QFileInfo > qms( d.entryInfoList( QStringList( "*.qm" ), QDir::Files ) );
		foreach( QFileInfo qm, qms ) {
			TranslationMap::mapped_type tmp( new QTranslator );
			tmp->load( qm.absoluteFilePath() );
			translations().insert( qm.baseName(), tmp );
		}
	}

	const TranslationMap & getTranslations() {
		return translations();
	}

	const QStringList & SupportedFormats() {
		static QStringList sf = uniqueList();
		return sf;
	}

	const QStringList & SupportedFormatsFilter() {
		static QStringList sff = toNameFilter( KomiX::SupportedFormats() );
		return sff;
	}

	QStringList toNameFilter( const QStringList & exts ) {
		QStringList tmp;
		foreach( QString str, exts ) {
			tmp << str.prepend( "*." );
		}
		return tmp;
	}

}
