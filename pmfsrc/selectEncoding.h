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
#include "gxml.hpp"

#ifndef _SELECT_ENCODING_
#define _SELECT_ENCODING_

class SelectEncoding : public QDialog
{
    Q_OBJECT
public:
    SelectEncoding(DSQLPlugin* pDSQL, QWidget *parent=0);
    ~SelectEncoding();
    GString encoding();
    static GString getEncoding(DSQLPlugin* pDSQL);
    static int convertToXml();


private slots:
    void OKClicked();
    void CancelClicked();
    void keyPressEvent(QKeyEvent *event);

private:
    QPushButton * ok, * cancel;
    QComboBox * m_encListCB;
    QRadioButton * m_pSaveAllDB, *m_pSaveThisDB, *m_pThisSession;
    DSQLPlugin* m_pDSQL;
    void msg(GString message);
    GString getCurrentEncoding(GString cmd);
    static GString getEncFileName();
    int saveEncoding();
    static void createMissingXml();
    int saveAsXml();
    static GString getEncodingFromXml(DSQLPlugin* pDSQL);

    GString m_selectedEncoding;
};

#endif
