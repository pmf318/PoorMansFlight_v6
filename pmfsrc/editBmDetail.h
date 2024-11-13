//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include <qapplication.h>
#include <qpushbutton.h>
#include <qmainwindow.h>
#include <qlineedit.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <QListWidget>
#include <QCheckBox>

#include <gstring.hpp>
#include <qfiledialog.h>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>
#include <dsqlplugin.hpp>
#include <dbapiplugin.hpp>

#include <gstring.hpp>
#include "txtEdit.h"
#include "bookmark.h"

#ifndef _EDITBMDETAIL_
#define _EDITBMDETAIL_

class EditBmDetail : public QDialog
{
   Q_OBJECT
public:
   EditBmDetail(QString name, QWidget *parent=0);
   ~EditBmDetail();
   //short fillLB();
   void setText(GString text);
   void setBookmarkData(BOOKMARK * pBm, BookmarkSeq *bookmarkSeq);
   void getBookmarkData(BOOKMARK * pBm);
   void setSqlHighlighter(PmfColorScheme colorScheme, GSeq <GString>* list);
   void setLineWrapping(QTextEdit::LineWrapMode mode);
   int saveClicked();

private slots:
   void OKClicked();
   void CancelClicked();
   void displayData();

private:
    BookmarkSeq * _bookmarkSeq;
    BOOKMARK * _pBm;
   void reject();
   QPushButton * ok, *cancel;
   QLineEdit *nameLE, *tableLE;
   QListWidget* infoLB;
   TxtEdit * infoTE;
   GString m_gstrOrgText;
   QString m_gstrSettingsName;
   int okClicked;
};

#endif
