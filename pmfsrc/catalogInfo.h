//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#ifndef _catalogInfo_
#define _catalogInfo_
#include <QMessageBox>

#include <dsqlplugin.hpp>
#include <dbapiplugin.hpp>

#define ADD_CATINFO_MODE_NORM 0
#define ADD_CATINFO_MODE_EMBED 1

class CatalogInfo : public QDialog
{
   Q_OBJECT
public:
    CatalogInfo(DSQLPlugin * pDSQL, QWidget *parent=0);
    ~CatalogInfo();
   short fillLV();
   void createDisplay(int mode = ADD_CATINFO_MODE_NORM);

public slots:
   void OKClicked();

private slots:
   void newClicked();
   void uncDatabaseClicked();
   void uncNodeClicked();
   void sortClicked(int);  
protected:
    void keyPressEvent(QKeyEvent *event);

private:
   void uncatalogItem(int colNr);
   int uncatalogNode(DBAPIPlugin* pApi, GString name);
   int uncatalogDB(DBAPIPlugin* pApi, GString name, GString node);
   void refreshDirectory();
   NODE_INFO * nodeFromList(GSeq<NODE_INFO*> *nodeSeq,  GString nodeName);
   QTableWidgetItem * createItem(GString text);
   void tm(GString message){QMessageBox::information(this, "PMF", message);}
   QPushButton * ok, *newB, *uncNodeB, *uncDatabaseB;
   QTableWidget* mainLV;
   QLabel * info;
   DBAPIPlugin * pApi;
   DSQLPlugin * m_pIDSQL;
   QWidget* m_pParent;
   int m_iMode;

};

#endif
