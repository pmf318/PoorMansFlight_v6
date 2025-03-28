//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include "bookmark.h"
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QDir>
#include <qlayout.h>
#include "gxml.hpp"
#include "helper.h"

#include <gstuff.hpp>
#include <gdebug.hpp>
#include <qfont.h>
#include <gfile.hpp>

/*******************************************************************************
 *
 *
 * CLASS BookmarkSeq
 *
 *
 ******************************************************************************/

BookmarkSeq::BookmarkSeq(GDebug * pGDeb)
{
    m_pGDeb = pGDeb;
    readAll();
}

BookmarkSeq::~BookmarkSeq()
{
    m_seqBookmark.deleteAll();
}

void BookmarkSeq::readAll()
{
    deb("readAll start");
    GString data;
    QString home = QDir::homePath ();
    if( !home.length() ) return;
    deb("bmFile: "+bmFileName());

//    GString bn = bmFileName();
//    int res = access( bmFileName(), F_OK );
//    if( !Helper::fileExists(bmFileName())) return 0;
//    GXml gx;
//    gx.readFromFile(bmFileName());

//    GXml bmXml = gx.getBlocksFromXPath("BOOKMARKS");
//    int count = bmXml.countBlocks("bookmark");
//    for(int i=1; i <= count; ++i )
//    {
//        name = bmXml.getBlockAtPosition("bookmark", i).getAttribute("Name");
//        GXml sqlXml = bmXml.getBlockAtPosition("bookmark", i).getBlocksFromXPath("SQL");
//        sql = sqlXml.toString().stripLeading("<SQL>").stripTrailing("</SQL>");

//    }

    deb("Open file...");
    GFile f( bmFileName(), GF_APPENDCREATE);
    BOOKMARK * pBm;
    for( int i = 1; i <=f.lines(); ++i )
    {
        data = f.getLine(i);
        deb("BM_PURE_LINE: "+data);
        if( data.occurrencesOf(_PMF_PASTE_SEP) != 2 ) continue;
        pBm = new BOOKMARK;
        pBm->Name = data.subString(1, data.indexOf(_PMF_PASTE_SEP)-1).strip();
        data = data.remove(1, data.indexOf(_PMF_PASTE_SEP)+_PMF_PASTE_SEP.length()-1);
        pBm->Table = data.subString(1, data.indexOf(_PMF_PASTE_SEP)-1).strip();
        data = data.remove(1, data.indexOf(_PMF_PASTE_SEP)+_PMF_PASTE_SEP.length()-1);
        deb("BM_PURE_DATA: "+data);
        pBm->SqlCmd = data.change(_CRLF_MARK, "\n");
        m_seqBookmark.add(pBm);
    }
    deb("readAll done.");
}

int BookmarkSeq::count()
{
    return m_seqBookmark.numberOfElements();
}

int BookmarkSeq::addBookmark(GString name, GString table, GString sqlCmd)
{
    BOOKMARK *pBm = new BOOKMARK;
    pBm->Name = name;
    pBm->Table = table;
    pBm->SqlCmd = sqlCmd;

    for(int i = 1; i <= m_seqBookmark.numberOfElements(); ++i )
    {
        if( m_seqBookmark.elementAtPosition(i)->Name == name )
        {
            m_seqBookmark.replaceAt(i, pBm);
            return 0;
        }
    }
    m_seqBookmark.add(pBm);
    return 0;
}

int BookmarkSeq::removeBookmark(GString name)
{
    for(int i = 1; i <= m_seqBookmark.numberOfElements(); ++i )
    {
        if( m_seqBookmark.elementAtPosition(i)->Name == name )
        {
            m_seqBookmark.removeAt(i);
            return 0;
        }
    }
    return 1;
}

int BookmarkSeq::saveSeq()
{
    GFile *file;
    QString home = QDir::homePath ();
    if( !home.length() ) return 2;
    if( !Helper::fileExists(bmFileName()) ) file = new GFile(bmFileName(), GF_APPENDCREATE);
    else file = new GFile( bmFileName() );

    BOOKMARK * pBm;
    GSeq<GString> seqAllBM;
    for( unsigned int i = 1; i <= m_seqBookmark.numberOfElements(); ++i )
    {
        pBm = m_seqBookmark.elementAtPosition(i);
        seqAllBM.add(pBm->Name + _PMF_PASTE_SEP + pBm->Table + _PMF_PASTE_SEP + (pBm->SqlCmd).change("\n", _CRLF_MARK));
    }
    file->overwrite(&seqAllBM);
    delete file;

    return 0;
}

BOOKMARK * BookmarkSeq::getBookmark(GString name)
{
    for( unsigned int i = 1; i <= m_seqBookmark.numberOfElements(); ++i )
    {
        if( m_seqBookmark.elementAtPosition(i)->Name == name )
        {
            return  m_seqBookmark.elementAtPosition(i);
        }
    }
    return NULL;
}

BOOKMARK * BookmarkSeq::getBookmark(int pos)
{
    if( pos < 1 || pos > (signed) m_seqBookmark.numberOfElements() ) return NULL;
    return  m_seqBookmark.elementAtPosition(pos);
}


GString BookmarkSeq::getBookmarkName(int i)
{
    if( i < 1 || i > (signed) m_seqBookmark.numberOfElements() ) return "";
    return m_seqBookmark.elementAtPosition(i)->Name;
}
int BookmarkSeq::checkNameExists(GString bmName)
{
    BOOKMARK * pBm;
    for( unsigned int i = 1; i <= m_seqBookmark.numberOfElements(); ++i )
    {
        pBm = m_seqBookmark.elementAtPosition(i);
        if( pBm->Name == bmName ) return 1;
    }
    return 0;
}
GString BookmarkSeq::bmFileName()
{
    deb("bmFileName start");
    QString home = QDir::homePath ();
    if( !home.length() ) return "";
    return basePath() + _BM_FILE;
}

void BookmarkSeq::deb(GString msg)
{
    if( m_pGDeb ) m_pGDeb->debugMsg("bookmark", 1, msg);
}

/*******************************************************************************
 *
 *
 * CLASS Bookmark
 *
 *
 ******************************************************************************/




Bookmark::Bookmark( GDebug * pGDeb, QWidget* parent, GString sql, GString table, GString bmName )
: QDialog(parent)
{
    m_pGDeb = pGDeb;
	deb("ctor");
	m_strSQL = sql;
	m_strTable = table;
	if( table.strip(".") == _selStringCB ) table= "";
	
	//Read bookmarks from file
    _pBmSeq = new BookmarkSeq();
	
	//No GUI, just provide r/w access to bookmarks
	if( !parent ) return;

    if( bmName.strip().length() ) _bmSaveName = bmName;
    else _bmSaveName = "";

	this->setWindowTitle("Add bookmark");
	QVBoxLayout *topLayout = new QVBoxLayout( );
	
	QGridLayout *grid = new QGridLayout(this);
	topLayout->addLayout( grid, 10 );
	
    QLabel* tmpQLabel0 = new QLabel( this );
    tmpQLabel0->setText( "Name:" );
    //tmpQLabel0->setFixedHeight( tmpQLabel0->sizeHint().height() );
    grid->addWidget(tmpQLabel0, 0, 0);
	nameLE = new QLineEdit( this );
    nameLE->setText(bmName);
    grid->addWidget(nameLE, 0, 1, 1, 2);

	
    QLabel* tmpQLabel1 = new QLabel( this);
    tmpQLabel1->setText( "Table:" );
    grid->addWidget(tmpQLabel1, 1, 0);
    tableLE = new QLineEdit( this );
    tableLE->setPlaceholderText("[Optional]");
    grid->addWidget(tableLE, 1, 1, 1, 2);
	
    QLabel* tmpQLabel2 = new QLabel( this );
    tmpQLabel2->setText( "SQL CMD:" );
    grid->addWidget(tmpQLabel2, 2, 0);
	sqlLE = new QLineEdit( this );
    grid->addWidget(sqlLE, 2, 1, 1, 2);


	
	grid->setColumnStretch(0, 0);
	grid->setColumnStretch(1, 100);
	grid->setColumnStretch(2, 100);
    tableLE->setText(table);
	sqlLE->setText(sql);
	
	
	okB = new QPushButton();
	connect( okB, SIGNAL(clicked()), SLOT(okClicked()) );
	okB->setText( "OK" );
    okB->setAutoRepeat( false );
    //okB->setAutoResize( false );
    okB->setDefault(true);
	okB->setFixedHeight( okB->sizeHint().height() );
    grid->addWidget(okB, 3, 1);
	cancelB = new QPushButton();
	connect( cancelB, SIGNAL(clicked()), SLOT(cancelClicked()) );
	cancelB->setText( "Cancel" );
    cancelB->setAutoRepeat( false );
    //cancelB->setAutoResize( false );
	cancelB->setFixedHeight( cancelB->sizeHint().height() );
    grid->addWidget(cancelB, 3, 2);
	
	setGeometry(parent->pos().rx()+parent->width()/2 - 185,parent->pos().ry()+30, 370, 160);
	//resize( 270, 160 );
}


Bookmark::~Bookmark()
{
    delete _pBmSeq;
}

void Bookmark::okClicked()
{
    m_strName = "";
    GString name = nameLE->text();
	if( !name.length() ) 
	{
		msg("Please set a name.");
		nameLE->setFocus();
		return;
	}
    if( _pBmSeq->checkNameExists(name) && _bmSaveName != name )
    {
        msg("There is already a bookmark with this name.");
        return;
    }
    m_strName = name;
    _pBmSeq->addBookmark(name, m_strTable, m_strSQL);
    _pBmSeq->saveSeq();
	close();
}
void Bookmark::keyPressEvent(QKeyEvent *event)
{
	switch(event->key())
	{
		case Qt::Key_Escape:
			cancelClicked();
			break;
			
		case Qt::Key_Return:
			okClicked();
			break;
			
	}
	event->accept();
}
void Bookmark::closeEvent(QCloseEvent * event)
{
	event->accept();
}

GString Bookmark::getSavedName()
{
    return m_strName;
}

void Bookmark::cancelClicked()
{
    m_strName = "";
	close();
}

void Bookmark::msg(GString txt)
{
    QMessageBox::information(this, "Bookmark", txt);
}


void Bookmark::deb(GString msg)
{    
    m_pGDeb->debugMsg("bookmark", 1, msg);
}
