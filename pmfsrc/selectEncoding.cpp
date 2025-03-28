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

static GString ENC_ALL_DB =  "ENC_ALL_DB";

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
    m_pDSQL->setCLOBReader(1);
    GString err = m_pDSQL->initAll(cmd);
    m_pDSQL->setCLOBReader(0);
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

GString SelectEncoding::getEncFileName()
{
    QString home = QDir::homePath ();
    if( !home.length() ) return "";
    GString path;
#if defined(MAKE_VC) || defined (__MINGW32__)
    path = GString(home)+"\\"+_CFG_DIR+"\\"+_ENCODINGS_FILE+".xml";
#else
    path = GString(home)+"/."+_CFG_DIR + "/"+_ENCODINGS_FILE+".xml";
#endif
    return path;
}

GString SelectEncoding::getEncodingFromXml(DSQLPlugin* pDSQL)
{
    CON_SET curSet;
    pDSQL->currentConnectionValues(&curSet);
    GXml fXml;
    int erc = fXml.readFromFile(SelectEncoding::getEncFileName());
    if( erc ) return NULL;

    GXmlNode *encNodes = fXml.nodeFromXPath("/Encodings");
    if( encNodes == NULL ) return NULL;

    int count = encNodes->childCount();
    for(int i=1; i <= count; ++i )
    {
        GXmlNode *encNode = encNodes->childAtPosition(i);
        if( encNode->tagName() != "Encoding" ) continue;
        if( encNode->getAttributeValue("dbtype") == curSet.Type && encNode->getAttributeValue("host") == curSet.Host &&
            encNode->getAttributeValue("port") == curSet.Port && encNode->getAttributeValue("db") == curSet.DB &&
            encNode->getAttributeValue("user") == curSet.UID   )
        {
            return encNode->getAttributeValue("encoding");
        }
    }
    return "";
}

void SelectEncoding::createMissingXml()
{
    GFile f(SelectEncoding::getEncFileName());
    if( f.initOK() ) return;

    GXml gXml;
    gXml.create();
    GXmlNode * rootNode = gXml.getRootNode();
    rootNode->addNode("Encodings");

    GFile out( SelectEncoding::getEncFileName(), GF_OVERWRITE);
    if( out.initOK() )    {
        out.addLine(gXml.toString());
    }
}

int SelectEncoding::saveAsXml()
{
    SelectEncoding::createMissingXml();
    GXml gXml;
    gXml.readFromFile(SelectEncoding::getEncFileName());

    CON_SET curSet;
    m_pDSQL->currentConnectionValues(&curSet);
    curSet.CltEnc = GString(m_encListCB->currentText());

    GXmlNode *encNodes = gXml.nodeFromXPath("/Encodings");
    GXmlNode *encNode;
    if( encNodes == NULL ) return 2;

    int found = 0;
    int count = encNodes->childCount();
    for(int i=1; i <= count; ++i )
    {
        encNode = encNodes->childAtPosition(i);
        if( encNode->tagName() != "Encoding" ) continue;
        if( encNode->getAttributeValue("dbtype") == curSet.Type && encNode->getAttributeValue("host") == curSet.Host &&
            encNode->getAttributeValue("port") == curSet.Port && encNode->getAttributeValue("db") == curSet.DB &&
            encNode->getAttributeValue("user") == curSet.UID   )
        {
            encNode->setAttributeValue("encoding", curSet.CltEnc);
            found = 1;
        }
    }
    if( !found )
    {
        encNode = encNodes->addNode("Encoding");
        encNode->addAttribute("dbtype", curSet.Type);
        encNode->addAttribute("host", curSet.Host);
        encNode->addAttribute("port", curSet.Port);
        encNode->addAttribute("db", curSet.DB);
        encNode->addAttribute("user", curSet.UID);
        encNode->addAttribute("encoding", curSet.CltEnc);
    }
    GFile f( SelectEncoding::getEncFileName(), GF_OVERWRITE );
    if( f.initOK() )    {
        f.addLine(gXml.toString());
    }
}

int SelectEncoding::convertToXml()
{
    GFile f, x;
    GString oldPath = basePath() + _ENCODINGS_FILE;
    GString xmlPath = basePath() + _ENCODINGS_FILE+".xml";
    x.readFile(xmlPath);
    //Already converted to xml
    if( x.initOK() ) return 0;

    SelectEncoding::createMissingXml();
    GXml gXml;
    gXml.readFromFile(SelectEncoding::getEncFileName());

    GString line;
    f.readFile(oldPath);
    CON_SET storeSet;

    GXmlNode *encNodes = gXml.nodeFromXPath("/Encodings");
    GXmlNode *encNode;

    for(int i = 1; i <= f.lines(); ++i )
    {
        line = f.getLine(i);
        if( Helper::connSetFromString(line, &storeSet) != 0 ) continue;
        encNode = encNodes->addNode("Encoding");
        encNode->addAttribute("dbtype", storeSet.Type);
        encNode->addAttribute("host", storeSet.Host);
        encNode->addAttribute("port", storeSet.Port);
        encNode->addAttribute("db", storeSet.DB);
        encNode->addAttribute("user", storeSet.UID);
        encNode->addAttribute("encoding", storeSet.CltEnc);
    }
    GFile out( SelectEncoding::getEncFileName(), GF_OVERWRITE );
    if( out.initOK() )    {
        out.addLine(gXml.toString());
    }
    return 0;
}

int SelectEncoding::saveEncoding()
{
    return saveAsXml();
    GString path;
    QString home = QDir::homePath ();
    if( !home.length() ) return 1;

    path = basePath() + _ENCODINGS_FILE;
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
                storeSet.Port == curSet.Port && storeSet.DB == curSet.DB && storeSet.UID == curSet.UID)
            {
                return f.replaceAt(i, Helper::connSetToString(&curSet));
            }
        }
    }
    f.addLine(Helper::connSetToString(&curSet));
}

GString SelectEncoding::getEncoding(DSQLPlugin* pDSQL )
{

    return getEncodingFromXml(pDSQL);
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
                storeSet.Port == curSet.Port && storeSet.DB == curSet.DB  && storeSet.UID == curSet.UID  )
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
