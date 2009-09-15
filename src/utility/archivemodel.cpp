#include "archivemodel.hpp"
#include "global.hpp"

#include <QProcess>
#include <QtDebug>
#include <QDir>
#include <QApplication>

namespace {

	KomiX::FileModel * create( const QFileInfo & path ) {
		return new KomiX::ArchiveModel( path );
	}

	bool check( const QFileInfo & path ) {
		if( !path.isDir() ) {
			return KomiX::isArchiveSupported( path.absolutePath() );
		} else {
			return false;
		}
	}

	static const bool registered = KomiX::FileModel::registerModel( check, create );

	QDir createTmpDir() {
		qsrand( qApp->applicationPid() );
		QString tmpPath( QString( "komix_%1" ).arg( qrand() ) );
		QDir tmpDir( QDir::temp() );
		if( !tmpDir.mkdir( tmpPath ) ) {
			qWarning( "can not make temp dir" );
		} else {
			tmpDir.cd( tmpPath );
		}
		return tmpDir;
	}

	int rmdir( QDir dir ) {
		int sum = 0;
		QFileInfoList entry = dir.entryInfoList( QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs );
		foreach( QFileInfo e, entry ) {
			if( e.isDir() ) {
				sum += rmdir( e.absoluteFilePath() );
			} else {
				if( QFile::remove( e.absoluteFilePath() ) ) {
					++sum;
				}
			}
		}
		dir.rmdir( dir.absolutePath() );
		return sum + 1;
	}

}

namespace KomiX {

	const QDir ArchiveModel::TmpDir_ = ::createTmpDir();

	ArchiveModel::ArchiveModel( const QFileInfo & root ) {
		QProcess * p = new QProcess();
		qDebug() << ( Arguments_( root.fileName() ) << root.absoluteFilePath() );
		p->start( SevenZip_(), ( Arguments_( root.fileName() ) << root.absoluteFilePath() ),  QIODevice::ReadOnly );
		p->waitForFinished( -1 );

		if( p->exitCode() != 0 ) {
			qWarning() << p->readAllStandardOutput();
			qWarning() << p->readAllStandardError();
			// TODO: ERROR HERE
		} else {
			QDir aDir = ArchiveDir_( root.fileName() );
			QStringList tmpList = aDir.entryList( SupportedFormatsFilter(), QDir::Files );
			dir_ = aDir;
			files_ = tmpList;
			index_ = 0;
		}
	}

	ArchiveModel::~ArchiveModel() {
		int ret = ::rmdir( TmpDir_ );
		qDebug() << ret;
	}

	const QString & ArchiveModel::SevenZip_() {
#ifdef Q_OS_WIN32
		static QString sz = "C:\\Program Files\\7-Zip\\7z.exe";
#elif defined( Q_OS_UNIX )
		static QString sz = "/usr/bin/7z";
#endif
		return sz;
	}

	QStringList ArchiveModel::Arguments_( const QString & fileName ) {
		QStringList args( "e" );
		args << QString( "-o%1" ).arg( ArchiveDir_( fileName ).absolutePath() );
		args << "-aos";
		return args;
	}

	QDir ArchiveModel::ArchiveDir_( const QString & dirName ) {
		if( !TmpDir_.exists( dirName ) ) {
			TmpDir_.mkdir( dirName );
		}
		QDir tmp( TmpDir_ );
		tmp.cd( dirName );
		return tmp;
	}

}
