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

class AddDatabaseHost : public QDialog
{
    Q_OBJECT
public:
    AddDatabaseHost(DSQLPlugin* pDSQL, GSeq <CON_SET*> *conSetList, QWidget *parent=0);
    ~AddDatabaseHost();


private slots:
    void OKClicked();
    void CancelClicked();
    void keyPressEvent(QKeyEvent *event);

private:
    QPushButton * ok, * cancel;
    DSQLPlugin * m_pDSQL;
    GSeq <CON_SET*> *m_pConSetList;
    QLineEdit * m_pHostLE, *m_pPortLE, *m_pUidLE, *m_pPwdLE;
    QCheckBox *m_excludeCkB;
    void msg(GString message);
};

#endif
