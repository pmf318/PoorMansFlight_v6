//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//


#include <qlayout.h>
#include <qfont.h>
//Added by qt3to4:
#include <QGridLayout>
#include <QLabel>
#include <QHeaderView>

#include "pmf.h"
#include "bookmark.h"
#include "helper.h"
#include "editBmDetail.h"

#ifndef _editBm_
#include "editBm.h"
#endif
EditBm::EditBm(GDebug *pGDeb, Pmf *parent)
  :QDialog(parent) //, f("Charter", 48, QFont::Bold)
{
    m_pGDeb = pGDeb;
	this->resize(480, 250);
	QGridLayout * grid = new QGridLayout(this);

    info = new QLabel(this);
    info->setText("Use Bookmarks->Add bookmark to add bookmarks");
    grid->addWidget(info, 0, 0, 1, 4);


    mainLV = new QTableWidget(this);
    grid->addWidget(mainLV, 2, 0, 1, 5);

	mainLV->setSelectionBehavior(QAbstractItemView::SelectRows);
	mainLV->setSelectionMode(QAbstractItemView::SingleSelection);

	exitButton = new QPushButton(this);
	exitButton->setText("Exit");
	connect(exitButton, SIGNAL(clicked()), SLOT(exitClicked()));

    editButton = new QPushButton(this);
    editButton->setText("Edit");
    connect(editButton, SIGNAL(clicked()), SLOT(editClicked()));


	delButton = new QPushButton(this);
	delButton->setText("Delete");
	connect(delButton, SIGNAL(clicked()), SLOT(delClicked()));

    grid->addWidget(delButton, 3, 0);
    grid->addWidget(editButton, 3, 1);
    grid->addWidget(exitButton, 3, 2);
    connect((QWidget*)mainLV->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sortClicked(int)));
    m_pPmf = parent;
	fillLV();

}
EditBm::~EditBm()
{
}

void EditBm::SaveSeq()
{
//    _bookmarks->clearBookmarkSeq();
//	for( int i = 0; i < mainLV->rowCount(); ++i )
//	{
//        _bookmarks->addBookmark(mainLV->item(i, 0)->text(), mainLV->item(i, 1)->text(), mainLV->item(i, 2)->text());
//	}
    _bookmarkSeq->saveSeq();
}
void EditBm::exitClicked()
{
	close();
}


short EditBm::fillLV()
{
    _bookmarkSeq = new BookmarkSeq(m_pGDeb);
	QTableWidgetItem * pItem;
    mainLV->setRowCount(_bookmarkSeq->count());
    mainLV->setColumnCount(2);

	mainLV->setHorizontalHeaderItem(0, new QTableWidgetItem("Name"));
    mainLV->setHorizontalHeaderItem(1, new QTableWidgetItem("Table"));
    mainLV->setHorizontalHeaderItem(2, new QTableWidgetItem("SQL-Statement"));
    for( int i = 1; i <= _bookmarkSeq->count(); ++i )
	{
        BOOKMARK *pBm = _bookmarkSeq->getBookmark(i);
        if( !pBm ) continue;
        pItem = new QTableWidgetItem((char*) pBm->Name);
        pItem->setFlags(pItem->flags() ^ Qt::ItemIsEditable);
        mainLV->setItem(i-1, 0, pItem);
        pItem = new QTableWidgetItem((char*) pBm->Table);
        pItem->setFlags(pItem->flags() ^ Qt::ItemIsEditable);
        mainLV->setItem(i-1, 1, pItem);

//        pItem->setData(Qt::DisplayRole, QString::fromLocal8Bit((char*) pBm->SqlCmd));
//        pItem = new QTableWidgetItem();
//        mainLV->setItem(i-1, 2, pItem);
	}
    //mainLV->resizeColumnsToContents();
#if QT_VERSION >= 0x050000
    mainLV->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
    mainLV->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif

    QObject::connect(mainLV, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(selDoubleClicked(QTableWidgetItem*)));
    Helper::setVHeader(mainLV);
	return 0;
}

void EditBm::selDoubleClicked(QTableWidgetItem *)
{
    editClicked();
}

void EditBm::delClicked()
{
	int pos = mainLV->currentRow();
	if( pos < 0 ) return;

	if( QMessageBox::question(this, "PMF", "Delete bokkmark '"+mainLV->item(pos, 0)->text()+"' ?", "Yes", "No", 0, 1) != 0 ) return;
    _bookmarkSeq->removeBookmark(mainLV->item(pos, 0)->text());
	mainLV->removeRow(mainLV->currentRow());
    SaveSeq();
}

void EditBm::editClicked()
{
    int pos = mainLV->currentRow();
    if( pos < 0 ) return;

    BOOKMARK * pBm;

    GString bmName = mainLV->item(pos, 0)->text();

    EditBmDetail *editBm = new EditBmDetail("BM_SQL", this);
    editBm->setSqlHighlighter(m_pPmf->getColorScheme(), m_pPmf->sqlCmdSeq());
    pBm = _bookmarkSeq->getBookmark(bmName);
    if( pBm == NULL ) return;

    editBm->setBookmarkData(pBm, _bookmarkSeq);
    editBm->exec();
    if( editBm->saveClicked() )
    {
        editBm->getBookmarkData(pBm);
        mainLV->item(pos, 0)->setText(pBm->Name);
        mainLV->item(pos, 1)->setText(pBm->Table);
    }
    delete editBm;
    SaveSeq();
}

void EditBm::sortClicked(int)
{
    Helper::setVHeader(mainLV);
}
