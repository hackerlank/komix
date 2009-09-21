#include "filecontroller.hpp"
#include "global.hpp"
#include "error.hpp"

#include <QMutexLocker>
#include <QFileInfo>
#include <QMutex>

namespace {

static inline QMutex * lock() {
	static QMutex m;
	return &m;
}

} // end of namespace

namespace KomiX { namespace private_ {

using model::FileModel;

FileController::FileController( QObject * parent ) :
QObject( parent ),
index_( 0 ),
model_( NULL ) {
}

bool FileController::open( const QUrl & url ) {
	QMutexLocker locker( lock() );
	try {
		model_ = FileModel::createModel( url );
	} catch( error::BasicError & e ) {
		emit errorOccured( e.getMessage() );
		return false;
	}
	if( isEmpty() ) {
		return false;
	} else {
		QModelIndex first = model_->index( url );
		if( first.isValid() ) {
			index_ = first.row();
		} else {
			first = model_->index( 0, 0 );
			index_ = 0;
		}
		emit imageLoaded( first.data( Qt::UserRole ).value< QPixmap >() );
		return true;
	}
}

QModelIndex FileController::getCurrentIndex() const {
	if( !isEmpty() ) {
		return model_->index( index_, 0 );
	} else {
		return QModelIndex();
	}
}

void FileController::next() {
	QMutexLocker locker( ::lock() );
	if( !isEmpty() ) {
		++index_;
		if( index_ >= model_->rowCount() ) {
			index_ = 0;
		}
		QModelIndex item = model_->index( index_, 0 );
		emit imageLoaded( item.data( Qt::UserRole ).value< QPixmap >() );
	}
}

void FileController::prev() {
	QMutexLocker locker( ::lock() );
	if( !isEmpty() ) {
		--index_;
		if( index_ < 0 ) {
			index_ = model_->rowCount() - 1;
		}
		QModelIndex item = model_->index( index_, 0 );
		emit imageLoaded( item.data( Qt::UserRole ).value< QPixmap >() );
	}
}

bool FileController::isEmpty() const {
	if( !model_ ) {
		return true;
	}
	return model_->rowCount() == 0;
}

QSharedPointer< FileModel > FileController::getModel() const {
	return model_;
}

} } // end of namespace
