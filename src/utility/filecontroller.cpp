#include "filecontroller.hpp"
#include "global.hpp"

#include <QMutexLocker>
#include <QtDebug>

namespace KomiX {

	FileControllerBase::FileControllerBase( int pfMax, int limit, QObject * parent ) :
	QObject( parent ),
	prefetchMax_( pfMax ),
	limit_( limit ),
	dir_(),
	files_(),
	index_( 0 ),
	history_(),
	cache_(),
	lock_() {
		if( limit_ < 0 ) {
			limit_ = 0;
		}
		if( prefetchMax_ < 0 ) {
			prefetchMax_ = 0;
		} else if( prefetchMax_ > limit_ ) {
			prefetchMax_ = limit_;
		}
	}

	bool FileControllerBase::open( const QString & filePath ) {
		QMutexLocker locker( &lock_ );
		if( !update_( filePath ) || files_.empty() ) {
			return false;
		} else {
			emit getImage( fetch_( dir_.filePath( files_[index_] ) ) );
			return true;
		}
	}

	void FileControllerBase::next() {
		QMutexLocker locker( &lock_ );
		if( !files_.empty() ) {
			++index_;
			if( index_ >= files_.size() ) {
				index_ = 0;
			}
			emit getImage( fetch_( dir_.filePath( files_[index_] ) ) );
			fetch_( dir_.filePath( files_[ (index_+prefetchMax_ >= files_.size()) ? index_+prefetchMax_-files_.size() : index_+prefetchMax_ ] ) );
		}
	}

	void FileControllerBase::prev() {
		QMutexLocker locker( &lock_ );
		if( !files_.empty() ) {
			--index_;
			if( index_ < 0 ) {
				index_ = files_.size() - 1;
			}
			emit getImage( fetch_( dir_.filePath( files_[index_] ) ) );
		}
	}

	bool FileControllerBase::isEmpty() const {
		return files_.empty();
	}
	
	void FileControllerBase::setPrefetchMax( int pfMax ) {
		prefetchMax_ = ( pfMax > limit_ ? limit_ : pfMax );
	}

	int FileControllerBase::getPrefetchMax() const {
		return prefetchMax_;
	}

	void FileControllerBase::setLimit( int limit ) {
		limit_ = ( limit < 0 ? 0 : limit );
	}

	int FileControllerBase::getLimit() const {
		return limit_;
	}
	
	QString FileControllerBase::getDirPath() const {
		return dir_.path();
	}
	
	QString FileControllerBase::getFilePath() const {
		return dir_.filePath( files_[index_] );
	}

	const QPixmap & FileControllerBase::getImage( const QString & filePath ) {
		QMutexLocker locker( &lock_ );
		return fetch_( filePath );
	}

	const QPixmap & FileControllerBase::fetch_( const QString & key ) {
		if( !cache_.contains( key ) ) {
// 			qDebug() << "Cache miss:" << key;
			cache_.insert( key, QPixmap( key ) );
			history_.enqueue( key );
			if( cache_.size() > limit_ ) {
				cache_.remove( history_.dequeue() );
			}
		} else {
// 			qDebug() << "Cache hit:" << key;
		}
		return cache_[key];
	}

	void FileControllerBase::prefetch_( int index ) {
// 		qDebug( "<FileControllerBase::prefetch_>" );
		for( int i = 1; i <= prefetchMax_; ++i ) {
			if( index + i >= files_.size() ) {
				index -= files_.size();
			}
			fetch_( dir_.filePath( files_[index+i] ) );
		}
// 		qDebug( "</FileControllerBase::prefetch_>" );
	}

	bool FileControllerBase::update_( const QString & filePath ) {
		QFileInfo tmp( filePath );
		if( tmp.isDir() ) {
			if( dir_ == tmp.absoluteFilePath() ) {
				return false;
			} else {
// 				qDebug() << tmp.absoluteFilePath();
// 				qDebug() << tmp.filePath();
// 				qDebug() << tmp.absoluteDir();
// 				qDebug() << tmp.dir();
				QStringList tmpList = QDir( tmp.absoluteFilePath() ).entryList( SupportedFormatsFilter(), QDir::Files );
				if( tmpList.isEmpty() ) {
					return false;
				}
				dir_ = tmp.absoluteFilePath();
				files_ = tmpList;
				index_ = 0;
			}
		} else {
			if( dir_ == tmp.dir() ) {
				if( files_[index_] == tmp.fileName() ) {
					return false;
				} else {
					index_ = files_.indexOf( tmp.fileName() );
				}
			} else {
				dir_ = tmp.dir();
				files_ = dir_.entryList( SupportedFormatsFilter(), QDir::Files );
				index_ = files_.indexOf( tmp.fileName() );
			}
		}
		prefetch_( index_ );
		return true;
	}

}
