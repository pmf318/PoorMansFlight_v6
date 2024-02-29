//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#ifndef _BOOKMARK_
#define _BOOKMARK_

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

#include <gstring.hpp>
#include <gseq.hpp>
#include <gdebug.hpp>
#include "pmfdefines.h"

typedef struct BK_MK{
    GString Name;
    GString Table;
    GString SqlCmd;
    void init(){Name = SqlCmd = Table = ""; }
} BOOKMARK;


class BookmarkSeq
{
private:
    GSeq <BOOKMARK*> m_seqBookmark;
    GDebug *m_pGDeb;
    void deb(GString msg);
    GString bmFileName();

public:
    BookmarkSeq(GDebug * pGDeb = NULL);
    ~BookmarkSeq();
    BOOKMARK* getBookmark(GString name);
    BOOKMARK* getBookmark(int pos);
    void readAll();
    int count();
    int addBookmark(GString name, GString table, GString sqlCmd);
    GString getBookmarkName(int i);
    int removeBookmark(GString name);
//    GString getBookmarkFormattedSQL(int i);
//    GString getBookmarkFormattedSQL(GString name);
//    GString getBookmarkFlatSQL(int i);
//    GString getBookmarkTable(int i);
//    BOOKMARK * getBookmarkByName(GString name);
//    int updateBookmarkByName(GString bmName, GString table, GString sql);
    int checkNameExists(GString bmName);

    int saveSeq();

};



class Bookmark : public QDialog
{

	Q_OBJECT
	GString dbName, userName, passWord, nodeName;
	
	public:
        Bookmark( GDebug *pGDeb, QWidget* parent = NULL, GString sql = "", GString table = "", GString bmName = "" );
        ~Bookmark();
		void tm(QString message){QMessageBox::information(this, "db-Connect", message);}
        GString getSavedName();
		
private:
		GString m_strSQL;
		GString m_strTable;
        GString m_strName;
		int readAll();
		void msg(GString txt);
		GString bmFileName();
		void deb(GString msg);
        GDebug *m_pGDeb;
        BookmarkSeq * _pBmSeq;
        GString _bmSaveName;

	protected slots:
		virtual void cancelClicked();
		virtual void okClicked();
		void closeEvent( QCloseEvent*);
		void keyPressEvent(QKeyEvent *event);
		
	protected:
		QComboBox* dbNameCB;
		QLineEdit* nameLE;
		QLineEdit* tableLE;
		QLineEdit* sqlLE;
		QPushButton* okB;
		QPushButton* cancelB;
		
};

#endif
