/**
 * @file imageview.cpp
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
#include "filecontroller.hpp"
#include "imageitem.hpp"
#include "imageview.hpp"
#include "navigator.hpp"
#include "scalewidget.hpp"

#include <QtCore/QParallelAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QSettings>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>

using namespace KomiX::widget;

ImageView::ImageView( QWidget * parent ):
QGraphicsView( parent ),
anime_( new QParallelAnimationGroup( this ) ),
controller_( new FileController( this ) ),
imgRatio_( 1.0 ),
imgRect_(),
msInterval_( 1 ),
navigator_( new Navigator( this ) ),
panel_( new ScaleWidget( this ) ),
pixelInterval_( 1 ),
pressEndPosition_(),
pressStartPosition_(),
scaleMode_( Custom ),
vpRect_(),
vpState_() {
	this->setScene( new QGraphicsScene( this ) );
	this->vpRect_ = this->mapToScene( this->viewport()->rect() ).boundingRect();

	QObject::connect( this->controller_, SIGNAL( imageLoaded( const QPixmap & ) ), this, SLOT( setImage( const QPixmap & ) ) );
	QObject::connect( this->controller_, SIGNAL( errorOccured( const QString & ) ), this, SIGNAL( errorOccured( const QString & ) ) );

	QObject::connect( this->navigator_, SIGNAL( required( const QModelIndex & ) ), this->controller_, SLOT( open( const QModelIndex & ) ) );

	QObject::connect( this, SIGNAL( scaled( int ) ), this->panel_, SLOT( scale( int ) ) );
	QObject::connect( this->panel_, SIGNAL( scaled( int ) ), this, SLOT( scale( int ) ) );
	QObject::connect( this->panel_, SIGNAL( fitHeight() ), this, SLOT( fitHeight() ) );
	QObject::connect( this->panel_, SIGNAL( fitWidth() ), this, SLOT( fitWidth() ) );
	QObject::connect( this->panel_, SIGNAL( fitWindow() ), this, SLOT( fitWindow() ) );

	QObject::connect( this->anime_, SIGNAL( stateChanged( QAbstractAnimation::State, QAbstractAnimation::State ) ), this, SLOT( animeStateChanged_( QAbstractAnimation::State, QAbstractAnimation::State ) ) );
}

bool ImageView::open( const QUrl & uri ) {
	return this->controller_->open( uri );
}

void ImageView::moveTo( Direction d ) {
	this->anime_->stop();
	QLineF v = this->getMotionVector_( d );
	this->moveBy_( v.p2() );
}

void ImageView::slideTo( Direction d ) {
	this->anime_->stop();
	QLineF v = this->getMotionVector_( d );
	int t = v.length() / this->pixelInterval_ * this->msInterval_;
	this->setupAnimation_( t, v.dx(), v.dy() );
	this->anime_->start();
}

void ImageView::moveBy( QPointF delta ) {
	delta /= this->imgRatio_;
	this->anime_->stop();
	QLineF v = this->getMotionVector_( delta.x(), delta.y() );
	this->moveBy_( v.p2() );
}

void ImageView::begin() {
	this->vpState_ = TopRight;
	this->moveTo( this->vpState_ );
}

void ImageView::end() {
	this->vpState_ = BottomLeft;
	this->moveTo( this->vpState_ );
}

void ImageView::fitHeight() {
	this->scale( this->vpRect_.height() / this->imgRect_.height() );
	this->scaleMode_ = Height;
}

void ImageView::fitWidth() {
	this->scale( this->vpRect_.width() / this->imgRect_.width() );
	this->scaleMode_ = Width;
}

void ImageView::fitWindow() {
	double dW = this->imgRect_.width() - this->vpRect_.width();
	double dH = this->imgRect_.height() - this->vpRect_.height();
	if( dW > dH ) {
		this->fitWidth();
	} else {
		this->fitHeight();
	}
	this->scaleMode_ = Window;
}

void ImageView::loadSettings() {
	QSettings ini;

	this->pixelInterval_ = ini.value( "pixel_interval", 1 ).toInt();
	this->msInterval_ = ini.value( "ms_interval", 1 ).toInt();
}

void ImageView::nextPage() {
	this->controller_->next();
}

void ImageView::previousPage() {
	this->controller_->prev();
}

void ImageView::scale( int pcRatio ) {
	if( pcRatio <= 0 || this->items().empty() ) {
		return;
	}

	this->scale( pcRatio / 100.0 / this->imgRatio_ );
	this->scaleMode_ = Custom;
}

void ImageView::scale( double ratio ) {
	this->scale( ratio, ratio );
	this->updateViewportRectangle_();
	this->moveBy();
	this->imgRatio_ *= ratio;
}

void ImageView::setImage( const QPixmap & pixmap ) {
	this->setImage( QList< QPixmap >() << pixmap );
}

void ImageView::setImage( const QList< QPixmap > & images ) {
	if( images.empty() ) {
		return;
	}
	// stop all movement
	this->anime_->stop();

	this->scene()->clear();
	this->anime_->clear();

	ImageItem * item = new ImageItem( images[0] );
	this->anime_->addAnimation( new QPropertyAnimation( item, "pos" ) );
	this->scene()->addItem( item );
	item->setTransformationMode( Qt::SmoothTransformation );
	this->imgRect_ = item->sceneBoundingRect();

	for( int i = 1; i < images.size(); ++i ) {
		item = new ImageItem( images[i] );
		this->anime_->addAnimation( new QPropertyAnimation( item, "pos" ) );
		this->scene()->addItem( item );
		item->setTransformationMode( Qt::SmoothTransformation );
		item->setPos( this->imgRect_.topLeft() + QPointF( images[i].width(), 0.0 ) );
		this->imgRect_ = this->imgRect_.united( item->sceneBoundingRect() );
	}

	this->updateViewportRectangle_();
	this->updateScaling_();
	this->begin();
}

void ImageView::showControlPanel() {
	this->panel_->show();
}

void ImageView::showNavigator() {
	if( this->controller_->isEmpty() ) {
		emit this->errorOccured( tr( "No openable file." ) );
		return;
	}
	this->navigator_->setModel( this->controller_->getModel() );
	this->navigator_->setCurrentIndex( this->controller_->getCurrentIndex() );
	this->navigator_->exec();
}

void ImageView::smoothMove() {
	if( this->items().empty() ) {
		return;
	}
	this->anime_->stop();
	switch( this->vpState_ ) {
	case TopRight:
		this->vpState_ = BottomRight;
		if( this->imgRect_.bottom() > this->vpRect_.bottom() ) {
			this->slideTo( this->vpState_ );
		} else {
			this->smoothMove();
		}
		break;
	case BottomRight:
		this->vpState_ = TopLeft;
		if( this->imgRect_.left() < this->vpRect_.left() ) {
			this->slideTo( this->vpState_ );
		} else {
			this->smoothMove();
		}
		break;
	case TopLeft:
		this->vpState_ = BottomLeft;
		if( this->imgRect_.bottom() > this->vpRect_.bottom() ) {
			this->slideTo( this->vpState_ );
		} else {
			this->smoothMove();
		}
		break;
	case BottomLeft:
		this->controller_->next();
		break;
	}
}

void ImageView::smoothReversingMove() {
	if( this->items().empty() ) {
		return;
	}
	this->anime_->stop();
	switch( this->vpState_ ) {
	case BottomLeft:
		this->vpState_ = TopLeft;
		if( this->imgRect_.top() < this->vpRect_.top() ) {
			this->slideTo( this->vpState_ );
		} else {
			this->smoothReversingMove();
		}
		break;
	case TopLeft:
		this->vpState_ = BottomRight;
		if( this->imgRect_.right() > this->vpRect_.right() ) {
			this->slideTo( this->vpState_ );
		} else {
			this->smoothReversingMove();
		}
		break;
	case BottomRight:
		this->vpState_ = TopRight;
		if( this->imgRect_.top() < this->vpRect_.top() ) {
			this->slideTo( this->vpState_ );
		} else {
			this->smoothReversingMove();
		}
		break;
	case TopRight:
		this->controller_->prev();
		this->end();
		break;
	}
}

void ImageView::dragEnterEvent( QDragEnterEvent * event ) {
	if( event->mimeData()->hasFormat( "text/uri-list" ) ) {
		event->acceptProposedAction();
	}
}

void ImageView::dragMoveEvent( QDragMoveEvent * event ) {
	event->acceptProposedAction();
}

void ImageView::dropEvent( QDropEvent * event ) {
	if( event->mimeData()->hasUrls() ) {
		QList< QUrl > urlList = event->mimeData()->urls();

		if( !urlList.empty() ) {
			foreach( QUrl url, urlList ) {
				emit this->fileDropped( url );
			}
		}
	}
	event->acceptProposedAction();
}

void ImageView::keyPressEvent( QKeyEvent * event ) {
	if( event->key() == Qt::Key_Up ) {
		this->moveBy( QPoint( 0, 10 ) );
	} else if( event->key() == Qt::Key_Down ) {
		this->moveBy( QPoint( 0, -10 ) );
	} else if( event->key() == Qt::Key_Left ) {
		this->moveBy( QPoint( 10, 0 ) );
	} else if( event->key() == Qt::Key_Right ) {
		this->moveBy( QPoint( -10, 0 ) );
	} else {
		// nothing
	}
}

void ImageView::mouseMoveEvent( QMouseEvent * event ) {
	if( event->buttons() & Qt::LeftButton ) {	// left drag event
		// change cursor icon
		foreach( QGraphicsItem * item, this->scene()->items() ) {
			if( item->cursor().shape() == Qt::BlankCursor ) {
				item->setCursor( Qt::ClosedHandCursor );
			}
		}

		QPoint delta = event->pos() - this->pressEndPosition_;
		this->moveBy( delta );
		this->pressEndPosition_ = event->pos();	// update end point
	} else {
		foreach( QGraphicsItem * item, this->scene()->items() ) {
			if( item->cursor().shape() == Qt::BlankCursor ) {
				item->setCursor( Qt::OpenHandCursor );
			}
		}
	}
}

void ImageView::mousePressEvent( QMouseEvent * event ) {
	this->pressStartPosition_ = event->pos();
	this->pressEndPosition_ = event->pos();

	if( event->button() == Qt::LeftButton ) {
		foreach( QGraphicsItem * item, this->scene()->items() ) {
			item->setCursor( Qt::ClosedHandCursor );
		}
	}
}

void ImageView::mouseReleaseEvent( QMouseEvent * event ) {
	if( event->button() == Qt::LeftButton ) {
		if( this->pressStartPosition_ == event->pos() ) {
			this->smoothMove();
		}

		// update cursor icon
		foreach( QGraphicsItem * item, this->scene()->items() ) {
			if( item->cursor().shape() == Qt::ClosedHandCursor ) {
				item->setCursor( Qt::OpenHandCursor );
			}
		}
	} else if( event->button() == Qt::MidButton ) {
		if( this->pressStartPosition_ == event->pos() ) {
			if( event->modifiers() & Qt::ControlModifier ) {
				emit this->scaled( 0 );
			} else {
				emit this->middleClicked();
			}
		}
	} else if( event->button() == Qt::RightButton ) {
		if( this->pressStartPosition_ == event->pos() ) {
			this->smoothReversingMove();
		}
	} else {
		// no button?
	}
}

void ImageView::resizeEvent( QResizeEvent * event ) {
	this->QGraphicsView::resizeEvent( event );
	this->updateViewportRectangle_();
	this->updateScaling_();
}

void ImageView::wheelEvent( QWheelEvent * event ) {
	int delta = event->delta();
	if( event->modifiers() & Qt::ControlModifier ) {
		if( delta < 0 ) {
			emit this->scaled( -10 );
		} else if( delta > 0 ) {
			emit this->scaled( 10 );
		}
	} else {
		if( delta < 0 ) {
			this->controller_->next();
		} else if( delta > 0 ) {
			this->controller_->prev();
		}
	}
}

void ImageView::animeStateChanged_( QAbstractAnimation::State newState, QAbstractAnimation::State /*oldState*/ ) {
	if( newState == QAbstractAnimation::Stopped ) {
		this->imgRect_ = QRectF();
		foreach( QGraphicsItem * item, this->items() ) {
			this->imgRect_ = this->imgRect_.united( item->sceneBoundingRect() );
		}
	}
}

void ImageView::moveBy_( const QPointF & delta ) {
	foreach( QGraphicsItem * item, this->items() ) {
		item->moveBy( delta.x(), delta.y() );
	}
	this->imgRect_.translate( delta );
}

void ImageView::updateScaling_() {
	switch( this->scaleMode_ ) {
	case Custom:
		this->scale( 1.0 );
		break;
	case Width:
		this->fitWidth();
		break;
	case Height:
		this->fitHeight();
		break;
	case Window:
		this->fitWindow();
		break;
	default:
		;
	}
}

void ImageView::updateViewportRectangle_() {
	this->vpRect_ = this->mapToScene( this->viewport()->rect() ).boundingRect();
}

QLineF ImageView::getMotionVector_( Direction d ) {
	double dx = 0.0, dy = 0.0;
	switch( d ) {
		case Top:
			dy = this->vpRect_.top() - this->imgRect_.top();
			break;
		case Bottom:
			dy = this->vpRect_.bottom() - this->imgRect_.bottom();
			break;
		case Left:
			dx = this->vpRect_.left() - this->imgRect_.left();
			break;
		case Right:
			dx = this->vpRect_.right() - this->imgRect_.right();
			break;
		case TopRight:
			dx = this->vpRect_.right() - this->imgRect_.right();
			dy = this->vpRect_.top() - this->imgRect_.top();
			break;
		case BottomRight:
			dx = this->vpRect_.right() - this->imgRect_.right();
			dy = this->vpRect_.bottom() - this->imgRect_.bottom();
			break;
		case TopLeft:
			dx = this->vpRect_.left() - this->imgRect_.left();
			dy = this->vpRect_.top() - this->imgRect_.top();
			break;
		case BottomLeft:
			dx = this->vpRect_.left() - this->imgRect_.left();
			dy = this->vpRect_.bottom() - this->imgRect_.bottom();
			break;
	}
	return this->getMotionVector_( dx, dy );
}

QLineF ImageView::getMotionVector_( double dx, double dy ) {
	QRectF req = this->imgRect_.translated( dx, dy );
	// horizontal
	if( this->imgRect_.width() < this->vpRect_.width() ) {
		dx += this->vpRect_.center().x() - req.center().x();
	} else if( req.left() > this->vpRect_.left() ) {
		dx += this->vpRect_.left() - req.left();
	} else if( req.right() < this->vpRect_.right() ) {
		dx += this->vpRect_.right() - req.right();
	}
	// vertical
	if( this->imgRect_.height() < this->vpRect_.height() ) {
		dy += this->vpRect_.center().y() - req.center().y();
	} else if( req.top() > this->vpRect_.top() ) {
		dy += this->vpRect_.top() - req.top();
	} else if( req.bottom() < this->vpRect_.bottom() ) {
		dy += this->vpRect_.bottom() - req.bottom();
	}
	return QLineF( 0, 0, dx, dy );
}

void ImageView::setupAnimation_( int msDuration, double dx, double dy ) {
	QList< QGraphicsItem * > items = this->items();
	for( int i = 0; i < items.length(); ++i ) {
		QGraphicsItem * item = items.at( i );
		QPropertyAnimation * anime = qobject_cast< QPropertyAnimation * >( this->anime_->animationAt( i ) );
		anime->setDuration( msDuration );
		anime->setStartValue( item->pos() );
		anime->setEndValue( item->pos() + QPointF( dx, dy ) );
	}
}
