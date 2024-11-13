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

#ifndef _EDIT_CONN_
#define _EDIT_CONN_

#define ADD_PGSQL_MODE_NORM 0
#define ADD_PGSQL_MODE_EMBED 1

class ConnSet;


class EditConn : public QDialog
{
    Q_OBJECT
public:
    EditConn(CON_SET* conSet, CON_SET* newCS, ConnSet *parent=0);
    ~EditConn();
    void CreateDisplay(int locked = 0);
    int hasChanged();

public slots:
    void OKClicked();

private slots:
    void cancelClicked();
    void keyPressEvent(QKeyEvent *event);
    void togglePwdClicked();
    void selectColorClicked();
    void resetColorClicked();
    void testConnClicked();

private:
    ConnSet * m_pParent;
    QPushButton * okBt, * cancel, *viewPwdBt, *selColorBt, *resetColorBt, *testConnBt;
    QRadioButton *_noSSLRB, *_allowSSLRB, *_preferSSLRB, *_requireSSLRB;
    CON_SET *m_pConSet;
    QLineEdit *m_pDbLE, *m_pHostLE, *m_pPortLE, *m_pUidLE, *m_pPwdLE, *m_pPwdCmdLE, *m_pReconnLE;
    QComboBox* m_pTypeCB;
    QLabel * m_infoLBL;
    QCheckBox *m_excludeCkB;
    CON_SET * m_pNewCS;
    int m_iHasChanged;
    int m_iLocked;
    void msg(GString message);
    void setColorFromString(GString rgbColor);
    void setColor(QColor color);
    void setNewCsData();
};

#endif
