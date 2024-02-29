//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include <qapplication.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <gstring.hpp>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qmessagebox.h>
#include <QTableWidget>
#include "pmfSchemaCB.h"
#include "bookmark.h"

#ifndef _editBm_
#define _editBm_


class Pmf;

class EditBm : public QDialog
{
   Q_OBJECT
public:
   EditBm(GDebug*pGDeb, Pmf *parent);
   ~EditBm();
   short fillLV();

private slots:   
	void exitClicked();
	void delClicked();	
    void sortClicked(int);
    void editClicked();
    void selDoubleClicked(QTableWidgetItem*);
private:
   void tm(QString message){QMessageBox::information(this, "Table sizes", message);}
   void SaveSeq();
   GString iTabName, iTabSchema;
   QPushButton * exitButton, *delButton, *editButton;
   QTableWidget* mainLV;
   QLabel * info;
   GDebug * m_pGDeb;
   BookmarkSeq * _bookmarkSeq;
   Pmf* m_pPmf;
};

#endif
