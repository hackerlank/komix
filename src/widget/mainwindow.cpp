#include "mainwindow.hpp"
#include "scaleimage.hpp"
#include "imagearea.hpp"
#include "preview.hpp"
#include "global.hpp"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QImageReader>
#include <QIcon>
#include <QApplication>
#include <QtDebug>

namespace KomiX {
	
	const QString MainWindow::fileFilter_ = SupportedFormatsFilter().join( " " );
	
	MainWindow::MainWindow( QWidget * parent, Qt::WindowFlags flags ) :
	QMainWindow( parent, flags ),
	imageArea_( new ImageArea( this ) ),
	scaleImage_( new ScaleImage( this ) ),
	preview_( new Preview( this ) ),
	trayIcon_( new QSystemTrayIcon( QIcon( ":/image/logo.svg" ), this ) ),
	index_( 0 ),
	dir_( QDir::home() ),
	files_(),
	dumpState_( Qt::WindowNoState ) {
		initMenuBar_();
		initCentralWidget_();
		initTrayIcon_();
		
		scaleImage_->setWindowTitle( tr( "Scale Image" ) );
		connect( scaleImage_, SIGNAL( scaled( int ) ), imageArea_, SLOT( scale( int ) ) );

		connect( preview_, SIGNAL( open( const QString & ) ), this, SLOT( open( const QString & ) ) );
	}
	
	void MainWindow::initMenuBar_() {
		QMenuBar * menuBar = new QMenuBar( this );
		
		QMenu * fileMenu = new QMenu( tr( "&File" ), menuBar );
		
		QAction * openImage = new QAction( tr( "&Open Image File" ), this );
		openImage->setShortcut( tr( "Ctrl+O" ) );
		connect( openImage, SIGNAL( triggered() ), this, SLOT( openFileDialog() ) );
		
		fileMenu->addAction( openImage );
		addAction( openImage );
		
		QAction * openDir = new QAction( tr( "Open &Directory" ), this );
		openDir->setShortcut( tr( "Ctrl+D" ) );
		connect( openDir, SIGNAL( triggered() ), this, SLOT( openDirDialog() ) );
		
		fileMenu->addAction( openDir );
		addAction( openDir );
		
		menuBar->addMenu( fileMenu );
		
		QMenu * view = new QMenu( tr( "&View" ), menuBar );
		
		QAction * fullScreen = new QAction( tr( "&Full Screen" ), this );
		fullScreen->setShortcut( Qt::Key_F11 );
		connect( fullScreen, SIGNAL( triggered() ), this, SLOT( toggleFullScreen() ) );
		
		view->addAction( fullScreen );
		addAction( fullScreen );

		QAction * hide = new QAction( tr( "&Hide window" ), this );
		hide->setShortcut( tr( "Esc" ) );
		connect( hide, SIGNAL( triggered() ), this, SLOT( toggleSystemTray() ) );

		view->addAction( hide );
		addAction( hide );

		view->addSeparator();
		
		QAction * scale = new QAction( tr( "&Scale image" ), this );
		scale->setShortcut( tr( "Ctrl+S" ) );
		connect( scale, SIGNAL( triggered() ), scaleImage_, SLOT( show() ) );
		
		view->addAction( scale );
		addAction( scale );
		
		menuBar->addMenu( view );
		
		QMenu * go = new QMenu( tr( "&Go" ), menuBar );

		QAction * jump = new QAction( tr( "&Jump to image" ), this );
		connect( jump, SIGNAL( triggered() ), this, SLOT( previewHelper_() ) );

		go->addAction( jump );
		
		QAction * prev = new QAction( tr( "&Preverse image" ), this );
		prev->setShortcut( Qt::Key_PageUp );
		connect( prev, SIGNAL( triggered() ), this, SLOT( prevFile() ) );
		
		go->addAction( prev );
		addAction( prev );
		
		QAction * next = new QAction( tr( "&Next image" ), this );
		next->setShortcut( Qt::Key_PageDown );
		connect( next, SIGNAL( triggered() ), this, SLOT( nextFile() ) );
		
		go->addAction( next );
		addAction( next );
		
		menuBar->addMenu( go );
		
		QMenu * help = new QMenu( tr( "&Help" ), menuBar );
		
		QAction * about__ = new QAction( tr( "&About" ), this );
		connect( about__, SIGNAL( triggered() ), this, SLOT( about() ) );
		
		help->addAction( about__ );

		QAction * aboutQt_ = new QAction( tr( "About &Qt" ), this );
		connect( aboutQt_, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );

		help->addAction( aboutQt_ );
		
		menuBar->addMenu( help );
		
		setMenuBar( menuBar );
	}
	
	void MainWindow::initCentralWidget_() {
		setCentralWidget( imageArea_ );
		
		imageArea_->setBackgroundRole( QPalette::Dark );
		imageArea_->setAlignment( Qt::AlignCenter );
		imageArea_->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
		imageArea_->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
		imageArea_->setAcceptDrops( true );
		
		connect( imageArea_, SIGNAL( wheelMoved( int ) ), this, SLOT( whellAction( int ) ) );
		connect( imageArea_, SIGNAL( nextPage() ), this, SLOT( nextFile() ) );
		connect( imageArea_, SIGNAL( fileDroped( const QString & ) ), this, SLOT( open( const QString & ) ) );
		connect( imageArea_, SIGNAL( middleClicked() ), this, SLOT( toggleFullScreen() ) );
	}

	void MainWindow::initTrayIcon_() {
		QMenu * menu = new QMenu( this );
		QAction * quit = menu->addAction( tr( "&Quit" ) );
		trayIcon_->setContextMenu( menu );

		trayIcon_->setToolTip( tr( "KomiX" ) );

		connect( quit, SIGNAL( triggered() ), qApp, SLOT( quit() ) );
		connect( trayIcon_, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ), this, SLOT( systemTrayHelper_( QSystemTrayIcon::ActivationReason ) ) );

		trayIcon_->show();
	}
	
	void MainWindow::systemTrayHelper_( QSystemTrayIcon::ActivationReason reason ) {
		switch( reason ) {
			case QSystemTrayIcon::Trigger:
				toggleSystemTray();
				break;
			default:
				;
		}
	}
	
	void MainWindow::previewHelper_() {
		if( files_.empty() ) {
			QMessageBox::information( this, tr( "No file to open" ), tr( "No openable file in this directory." ) );
		} else {
			preview_->listDirectory( dir_.path(), dir_.filePath( files_[index_] ) );
		}
	}
	
	void MainWindow::nextFile() {
		if( files_.size() ) {
			++index_;
			if( index_ >= files_.size() ) {
				index_ = 0;
			}
			imageArea_->openFile( dir_.filePath( files_[index_] ) );
		}
	}
	
	void MainWindow::prevFile() {
		if( files_.size() ) {
			--index_;
			if( index_ < 0 ) {
				index_ = files_.size() - 1;
			}
			imageArea_->openFile( dir_.filePath( files_[index_] ) );
		}
	}
	
	void MainWindow::whellAction( int delta ) {
		if( delta < 0 ) {
			nextFile();
		} else if( delta > 0 ) {
			prevFile();
		}
	}
	
	void MainWindow::updateEnvironment( const QString & name ) {
		qDebug( "void MainWindow::updateEnvironment( const QString & name )" );
		qDebug() << "name:" << name;
		
		QFileInfo temp( name );
		
		if( temp.isDir() ) {
			// Directory mode
			qDebug( "Directory mode" );
			qDebug() << "temp.absoluteDir:" << temp.absoluteDir();
			qDebug() << "temp.absoluteFilePath:" << temp.absoluteFilePath();
			
			dir_ = temp.absoluteFilePath();
			files_ = dir_.entryList( SupportedFormatsFilter(), QDir::Files );
			index_ = 0;
		} else {
			// File mode
			qDebug( "File mode" );
			
			dir_ = temp.dir();
			files_ = dir_.entryList( SupportedFormatsFilter(), QDir::Files );
			index_ = files_.indexOf( temp.fileName() );
		}
		
		qDebug() << "dir_:" << dir_;
	}
	
	void MainWindow::open( const QString & name ) {
		updateEnvironment( name );
		if( files_.empty() ) {
			QMessageBox::information( this, tr( "No file to open" ), tr( "No openable file in this directory." ) );
		} else {
			imageArea_->openFile( dir_.filePath( files_[index_] ) );
		}
	}
	
	void MainWindow::openFileDialog() {
		QString fileName = QFileDialog::getOpenFileName( this, tr( "Open image file" ), dir_.absolutePath(), fileFilter_ );
		if( !fileName.isEmpty() ) {
			open( fileName );
		}
	}
	
	void MainWindow::openDirDialog() {
		QString dirName = QFileDialog::getExistingDirectory( this, tr( "Open dicrectory" ), dir_.absolutePath() );
		if( !dirName.isEmpty() ) {
			open( dirName );
		}
	}
	
	void MainWindow::toggleFullScreen() {
		menuBar()->setVisible( !menuBar()->isVisible() );
		setWindowState( windowState() ^ Qt::WindowFullScreen );
	}
	
	void MainWindow::toggleSystemTray() {
		if( isVisible() ) {
			dumpState_ = windowState();
			hide();
		} else {
			show();
			setWindowState( dumpState_ );
		}
	}
	
	void MainWindow::about() {
		QMessageBox::about( this, tr( "About KomiX" ), tr( "Hello, world!" ) );
	}

}
