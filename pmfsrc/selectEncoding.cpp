//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include "selectEncoding.h"
#include "pmfdefines.h"
#include "gfile.hpp"
#include "helper.h"
#include <qlayout.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>



SelectEncoding::SelectEncoding(DSQLPlugin* pDSQL, QWidget *parent )
  :QDialog(parent) //, f("Charter", 48, QFont::Bold)
{
    this->resize(290, 200);
    this->setWindowTitle("Select encoding");
    GString clientEnc;

    m_pDSQL = pDSQL;

	QBoxLayout *topLayout = new QVBoxLayout(this);
	QGridLayout * grid = new QGridLayout();
    topLayout->addLayout(grid, 9);

	ok = new QPushButton(this);
    ok->setDefault(true);

	cancel = new QPushButton(this);
    ok ->setText("OK");
	ok->setFixedHeight( ok->sizeHint().height());
    cancel->setText("Cancel");
	cancel->setFixedHeight( cancel->sizeHint().height());
	connect(ok, SIGNAL(clicked()), SLOT(OKClicked()));
	connect(cancel, SIGNAL(clicked()), SLOT(CancelClicked()));

    int row = 0;
    grid->addWidget(new QLabel("Encoding that PMF uses for this DB") , row++, 0, 1, 4);
    //grid->addWidget(new QLabel("to be used for this DB") , row++, 0, 1, 3);


    clientEnc = getCurrentEncoding("SELECT current_setting('client_encoding')");

    row++;
    grid->addWidget(new QLabel("Client: ", this), row, 0);
    m_encListCB = new QComboBox(this);
    grid->addWidget(m_encListCB, row, 1, 1, 3);

    row++;
    grid->addWidget(new QLabel("Current settings: ", this), row, 0, 1, 3);

    QLineEdit * pSrvLE, *pCltLE;
    row++;
    grid->addWidget(new QLabel("   Server: ", this), row, 0);
    pSrvLE = new QLineEdit(getCurrentEncoding("SELECT current_setting('server_encoding')"), this);
    //pSrvLE->setEnabled(false);
    pSrvLE->setReadOnly(true);
    pSrvLE->setDisabled(true);
    grid->addWidget(pSrvLE, row, 1, 1, 3);
    row++;
    pCltLE = new QLineEdit(clientEnc, this);
    pCltLE->setReadOnly(true);
    pCltLE->setDisabled(true);

    grid->addWidget(new QLabel("   Client: ", this), row, 0);
    grid->addWidget(pCltLE, row, 1, 1, 3);


    m_pSaveAllDB = new QRadioButton("Save for all DBs of type "+pDSQL->getDBTypeName());
    m_pSaveThisDB = new QRadioButton("Make permanent");
    m_pThisSession = new QRadioButton("Only for this session");
    m_pSaveThisDB->setChecked(true);

    row++;
    //grid->addWidget(m_pSaveAllDB, 4, 0, 1, 4);
    grid->addWidget(m_pSaveThisDB, row, 0, 1, 2);
    grid->addWidget(m_pThisSession, row, 2, 1, 2);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QWidget * buttonWdiget = new QWidget();
    buttonWdiget->setLayout(buttonLayout);
    buttonLayout->addWidget(ok);
    buttonLayout->addWidget(cancel);

    row++;
    grid->addWidget(buttonWdiget, row, 0, 1, 3);

    GSeq <GString> encList;
    m_pDSQL->getAvailableEncodings(&encList);
    for( int i = 1; i <= encList.numberOfElements(); ++i )
    {
        m_encListCB->addItem(encList.elementAtPosition(i));
    }

    int pos = m_encListCB->findText(clientEnc);
    if( pos != -1 ) m_encListCB->setCurrentIndex(pos);
    m_selectedEncoding = clientEnc;

}

GString SelectEncoding::getCurrentEncoding(GString cmd)
{
    GString err = m_pDSQL->initAll(cmd);
    if( err.length()) return "<No Info>";
    return m_pDSQL->rowElement(1, 1 ).strip('\'');
}

SelectEncoding::~SelectEncoding()
{
	delete ok;
	delete cancel;
}

void SelectEncoding::OKClicked()
{
    if( m_pSaveThisDB->isChecked() ) saveEncoding();

    if( m_selectedEncoding == GString(m_encListCB->currentText() )) m_selectedEncoding = ""; //Nothing changed
    else m_selectedEncoding = GString(m_encListCB->currentText());
    close();
}

GString SelectEncoding::encoding()
{
    return m_selectedEncoding;
}

void SelectEncoding::msg(GString message)
{
    QMessageBox::information(this, "PMF", message);
}

void SelectEncoding::CancelClicked()
{
    m_selectedEncoding = "";
    close();
}
void SelectEncoding::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            OKClicked();
            break;
        case Qt::Key_Escape:
            CancelClicked();
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}

int SelectEncoding::saveEncoding()
{
    GString path;
    QString home = QDir::homePath ();
    if( !home.length() ) return 1;

#if defined(MAKE_VC) || defined (__MINGW32__)
    path = GString(home)+"\\"+_CFG_DIR+"\\"+_ENCODINGS_FILE;
#else
    path = GString(home)+"/."+_CFG_DIR + "/"+_ENCODINGS_FILE;
#endif
    GString line;
    GFile f;
    f.readFile(path);
    CON_SET curSet, storeSet;
    curSet.init();
    storeSet.init();

    m_pDSQL->currentConnectionValues(&curSet);
    curSet.CltEnc = GString(m_encListCB->currentText());

    for(int i = 1; i <= f.lines(); ++i )
    {
        line = f.getLine(i);
        if( Helper::connSetFromString(line, &storeSet) != 0 ) continue;
        if( m_pSaveAllDB->isChecked() )
        {
            curSet.DB = curSet.Port = curSet.Host = ENC_ALL_DB;
            if( storeSet.Type == curSet.Type )
            {
                f.removeAt(i);
            }
        }
        else if( m_pSaveThisDB )
        {
            if( storeSet.Type == curSet.Type && storeSet.Host == curSet.Host &&
                storeSet.Port == curSet.Port && storeSet.DB == curSet.DB )
            {
                return f.replaceAt(i, Helper::connSetToString(&curSet));
            }
        }
    }
    f.addLine(Helper::connSetToString(&curSet));
}

GString SelectEncoding::getEncoding(DSQLPlugin* pDSQL )
{
    QString home = QDir::homePath ();
    if( !home.length() ) return "";
    GString path;
#if defined(MAKE_VC) || defined (__MINGW32__)
    path = GString(home)+"\\"+_CFG_DIR+"\\"+_ENCODINGS_FILE;
#else
    path = GString(home)+"/."+_CFG_DIR + "/"+_ENCODINGS_FILE;
#endif

    GFile f;
    f.readFile(path);
    CON_SET curSet, storeSet;
    pDSQL->currentConnectionValues(&curSet);

    for(int i = 1; i <= f.lines(); ++i )
    {
        if( Helper::connSetFromString(f.getLine(i), &storeSet) == 0 )
        {
            if( storeSet.Type == curSet.Type && storeSet.Host == curSet.Host &&
                storeSet.Port == curSet.Port && storeSet.DB == curSet.DB )
            {
                return storeSet.CltEnc;
            }
        }
    }
    for(int i = 1; i <= f.lines(); ++i )
    {
        if( Helper::connSetFromString(f.getLine(i), &storeSet) == 0 )
        {
            if( storeSet.Type == curSet.Type )
            {
                return storeSet.CltEnc;
            }
        }
    }
    return "";
}
