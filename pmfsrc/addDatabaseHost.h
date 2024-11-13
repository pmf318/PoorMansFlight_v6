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
#include <QComboBox>
#include <QCheckBox>
#include <gstring.hpp>
#include <qfiledialog.h>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>

#include <gseq.hpp>


#include <dsqlplugin.hpp>
#include "pmfSchemaCB.h"
#include "tableSelector.h"

#ifndef _ADD_DATABASE_HOST_
#define _ADD_DATABASE_HOST_

#define ADD_PGSQL_MODE_NORM 0
#define ADD_PGSQL_MODE_EMBED 1


class AddDatabaseHost : public QDialog
{
    Q_OBJECT
public:
    AddDatabaseHost(DSQLPlugin* pDSQL, GSeq <CON_SET*> *conSetList, QWidget *parent=0);
    ~AddDatabaseHost();
    void CreateDisplay(int mode = ADD_PGSQL_MODE_NORM);

public slots:
    void OKClicked();

private slots:
    void CancelClicked();
    void keyPressEvent(QKeyEvent *event);
    void togglePwdClicked();
    void helpClicked();
    void testConnClicked();

private:
    QWidget * m_pParent;
    QPushButton * saveBt, * cancel, *viewPwdB, *testConnBt;
    QRadioButton *_noSSLRB, *_allowSSLRB, *_preferSSLRB, *_requireSSLRB;

    DSQLPlugin * m_pDSQL;
    int m_iMode;
    GSeq <CON_SET*> *m_pConSetList;
    QLineEdit * m_pHostLE, *m_pPortLE, *m_pUidLE, *m_pPwdLE, *m_pPwdCmdLE, *m_pReconnectTimeLE;
    QLabel * m_infoLBL;
    QCheckBox *m_excludeCkB;
    void msg(GString message);
};

#endif
