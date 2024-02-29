//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)
//

#include "importBox.h"
#include "getCodepage.h"
#include "helper.h"
#include "pmfdefines.h"
#include <qlayout.h>
#include <QGridLayout>
#include <QGroupBox>
#include <gfile.hpp>
#include <gstuff.hpp>
#include <dbapiplugin.hpp>
#include <QMimeData>
#include <QSettings>
#include <QList>
#include <QScrollBar>

#include "multiImport.h"


#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
//#   include <QUrlQuery>
#include <QUrl>
#endif


ImportBox::ImportBox(DSQLPlugin* pDSQL, QWidget *parent, GString currentSchema, GString currentTable, GString *prevDir, int hideSysTabs)
  :QDialog(parent) //, f("Charter", 48, QFont::Bold)
{

    if( !pDSQL ) return;
    m_pGDeb = NULL;

    m_iHideSysTables = hideSysTabs;
    m_sqlErc = 0;
    m_sqlErrTxt = "";
    this->currentSchema = currentSchema;
    this->currentTable = currentTable;



    m_pDSQL = new DSQLPlugin(*pDSQL);
    m_gstrPrevDir = prevDir;
    QGridLayout * grid = new QGridLayout( this );

    QDir qd;
    QString aPath = qd.homePath();
    //long ts = QDateTime::currentMSecsSinceEpoch();
    time_t now = time(0);

    #ifdef MAKE_VC
    m_strLogFile = GString(aPath)+"\\PMF_IMP_"+GString(now)+".LOG";
    m_strLogFile = m_strLogFile.change("/", "\\");
    #else
    m_strLogFile = GString(aPath)+"/.pmf6/PMF_IMP_"+GString(now)+".LOG";
    #endif


    ok = new QPushButton(this);
    ok->setDefault(true);
    cancel = new QPushButton(this);
    ok ->setText("Go!");
    cancel->setText("Exit");
    delLog = new QPushButton(this);
    delLog->setText("Remove ErrorLog");
    connect(ok, SIGNAL(clicked()), SLOT(OKClicked()));
    connect(cancel, SIGNAL(clicked()), SLOT(CancelClicked()));
    connect(delLog, SIGNAL(clicked()), SLOT(deleteErrLog()));

    //RadioButtons:
    typeGroup   = new QButtonGroup(this);
    importRB = new QRadioButton("Import", this);
    loadRB = new QRadioButton("Load", this);
    connect(importRB, SIGNAL(clicked()), this, SLOT(typeSelected()));
    connect(loadRB, SIGNAL(clicked()), this, SLOT(typeSelected()));
    importRB->setChecked(true);
    typeGroup->addButton(importRB);
    typeGroup->addButton(loadRB);
    optionsB = new QPushButton("Options");
    connect(optionsB, SIGNAL(clicked()), SLOT(optionsClicked()));
    if( pDSQL->getDBType() == DB2 || pDSQL->getDBType() == POSTGRES )  typeGroup->addButton(optionsB);
    else optionsB->hide();


    //RadioButtons:
    formGroup   = new QButtonGroup(this);
    ixfRB = new QRadioButton("IXF Format", this);
    delRB = new QRadioButton("DEL Format", this);
    wsfRB = new QRadioButton("WSF Format", this);
    csvRB = new QRadioButton("CSV Format", this);
    formGroup->addButton(ixfRB);
    formGroup->addButton(delRB);
    formGroup->addButton(wsfRB);
    formGroup->addButton(csvRB);




    actionGroup = new QButtonGroup(this);
    insRB = new QRadioButton("Insert into table", this);
    crtRB = new QRadioButton("Insert-Update table", this);
    updRB = new QRadioButton("Insert-Update table", this);
    replRB = new QRadioButton("Replace data", this);
    repCrRB = new QRadioButton("Create table (IXF only)", this);

    resetNoneRB = new QRadioButton("None", this );
    resetAllRB = new QRadioButton("All colums", this);
    resetAskRB = new QRadioButton("Ask", this);

    connect(insRB, SIGNAL(clicked()), this, SLOT(optionSelected()));
    connect(crtRB, SIGNAL(clicked()), this, SLOT(optionSelected()));
    connect(updRB, SIGNAL(clicked()), this, SLOT(optionSelected()));
    connect(replRB, SIGNAL(clicked()), this, SLOT(optionSelected()));
    connect(repCrRB, SIGNAL(clicked()), this, SLOT(optionSelected()));
    actionGroup->addButton(insRB);
    actionGroup->addButton(crtRB);
    actionGroup->addButton(updRB);
    actionGroup->addButton(replRB);
    actionGroup->addButton(repCrRB);

    tableNameLE = new QLineEdit(this);
    fileNameDropZone =  new PmfDropZone(this);
    tableLB = new QListWidget(this);
    errorLB = new QTextBrowser(this);
    getFileB = new QPushButton("Select File", this);
    connect(getFileB, SIGNAL(clicked()), SLOT(getFileClicked()));
    connect(fileNameDropZone, SIGNAL(fileWasDropped()), SLOT(filesDropped()));

    schemaCB = new PmfSchemaCB(this);


    //defaults
    insRB->setChecked(true);
    ixfRB->setChecked(true);
    if( pDSQL->getDBType() == POSTGRES )
    {
        ixfRB->setEnabled(false);
        loadRB->setEnabled(false);
        delRB->setEnabled(false);
        wsfRB->setEnabled(false);
        csvRB->setChecked(true);
    }
    //View ErrorLog

    QLabel * info = new QLabel(this);
    QFont fontBold  = info->font();
    fontBold.setBold(true);
    info->setText("Drag&drop file(s) into field below or click 'Select File'");
    grid->addWidget(info, 0, 0, 1, 4);
    fileNameDropZone->setPlaceholderText("Drag&drop file(s) here");

    QLabel * hint = new QLabel("Hint: Importing/Loading from LOCAL drives works best.");
    hint->setStyleSheet("font-weight: bold;");
    grid->addWidget(hint, 1, 0, 1, 4);

    //Row 1
    grid->addWidget(fileNameDropZone, 2, 0, 1, 2);
    grid->addWidget(getFileB, 2, 2);

    //Row 2
    QGroupBox * typeGroupBox = new QGroupBox();
    QHBoxLayout *typeLayout = new QHBoxLayout;
    typeGroupBox->setTitle("Import or Load");
    typeLayout ->addWidget(importRB);
    typeLayout ->addWidget(loadRB);
    typeLayout ->addWidget(optionsB);
    typeGroupBox->setLayout(typeLayout);
    grid->addWidget(typeGroupBox, 3, 0, 1, 4);

    //Row 3
    QGroupBox * formatGroupBox = new QGroupBox();
    QHBoxLayout *frmLayout = new QHBoxLayout;
    formatGroupBox->setTitle("File format");
    frmLayout->addWidget(ixfRB);
    frmLayout->addWidget(delRB);
    frmLayout->addWidget(wsfRB);
    if( pDSQL->getDBType() == POSTGRES ) frmLayout->addWidget(csvRB);
    else csvRB->hide();
    formatGroupBox->setLayout(frmLayout);
    grid->addWidget(formatGroupBox, 4, 0, 1, 4);

    //Row 4
    QGroupBox * resetGenGroupBox = new QGroupBox();
    QHBoxLayout *genLayout = new QHBoxLayout;
    resetGenGroupBox->setTitle("Reset GENERATED ALWAYS columns");
    genLayout->addWidget(resetNoneRB);
    genLayout->addWidget(resetAllRB);    
    genLayout->addWidget(resetAskRB);
    resetNoneRB->setChecked(true);
    if( !hasGeneratedColumns() )
    {
        resetNoneRB->setEnabled(false);
        resetAllRB->setEnabled(false);
        resetAskRB->setEnabled(false);
    }
    resetGenGroupBox->setLayout(genLayout);
    grid->addWidget(resetGenGroupBox, 5, 0, 1, 4);

    //left
    grid->addWidget(new QLabel("(Right-click buttons for help)"), 6, 0);
    grid->addWidget(insRB, 7, 0);
    grid->addWidget(replRB, 8, 0);
    grid->addWidget(updRB, 9, 0);
    grid->addWidget(crtRB, 10, 0);
    grid->addWidget(repCrRB, 11, 0);
    grid->addWidget(tableNameLE, 12, 0);
    QLabel *tbInfo = new QLabel("<-Tablename is case-sensitive!");
    grid->addWidget(tbInfo, 12, 1);


    //right
    grid->addWidget(schemaCB, 6, 1, 1, 2);
    //grid->addWidget()
    grid->addWidget(tableLB, 7, 1, 5, 2);

    //bottom
    grid->addWidget(errorLB, 14, 0, 1, 4);

    QGroupBox * btGroupBox = new QGroupBox();
    QHBoxLayout *btLayout = new QHBoxLayout;
    btLayout->addWidget(ok);
    btLayout->addWidget(cancel);
    btLayout->addWidget(delLog);
    btGroupBox->setLayout(btLayout);
    grid->addWidget(btGroupBox, 15, 0, 1, 4);

    insRB->setChecked(true);
    tableNameLE->setEnabled(false);
    tableNameLE->setPlaceholderText("table to create (or select from list)");
    setWhatsThisText();

    grid->setRowStretch(7, 90);
    connect(schemaCB, SIGNAL(activated(int)), SLOT(schemaSelected(int)));

    if( m_pDSQL->getDBType() == MARIADB )
    {
        ixfRB->setText("Standard");
        importRB->setDisabled(true);
        loadRB->setDisabled(true);
        ixfRB->setDisabled(true);
        wsfRB->setDisabled(true);
        delRB->setDisabled(true);
        insRB->setDisabled(true);
        updRB->setDisabled(true);
        replRB->setDisabled(true);
        repCrRB->setDisabled(true);
        crtRB->setDisabled(true);
    }


    schemaCB->fill(pDSQL, currentSchema, hideSysTabs);
    aThread = NULL;
    tb = NULL;
    timer = new QTimer( this );
    connect( timer, SIGNAL(timeout()), this, SLOT(timerEvent()) );


    if( parent->height() > 780 ) this->resize(610, 780);
    else this->resize(610, 580);
    this->move(parent->x() + (parent->width() - width()) / 2,  parent->y() + (parent->height() - height()) / 2);

    fillLB(currentSchema, currentTable);

    m_pExpImpOptions = NULL;
//  fillLB(tableNameSeq);
}
ImportBox::~ImportBox()
{
    //delete tb;
    delete ok;
    delete cancel;
    delete ixfRB;
    delete delRB;
    delete wsfRB;
    delete insRB;
    delete updRB;
    delete repCrRB;
    delete replRB;
    delete crtRB;
    delete tableLB;
    delete fileNameDropZone;
    delete tableNameLE;
    delete getFileB;
    delete formGroup;
    delete actionGroup;
    delete errorLB;
    delete delLog;
    delete schemaCB;
    delete timer;
    delete m_pDSQL;
    if( m_pExpImpOptions != NULL)delete m_pExpImpOptions;    
}

void ImportBox::setWhatsThisText()
{
    insRB->setWhatsThis("Insert data into existing table");
    crtRB->setWhatsThis("Create new table from IXF file. Enter table name below.");
    updRB->setWhatsThis("Existing rows will be updated, new rows inserted. A primary key is mandatory.");
    replRB->setWhatsThis("Replaces ALL data, EXISTING DATA WILL BE LOST.");
    repCrRB->setWhatsThis("(Re-)creates the table, EXISTING DATA WILL BE LOST. Enter table name below or select table from the right.");
}

void ImportBox::optionSelected()
{
    if( crtRB->isChecked() || repCrRB->isChecked() )
    {
        tableNameLE->setEnabled(true);
        tableLB->setEnabled(false);
    }
    else
    {
        tableNameLE->setEnabled(false);
        tableLB->setEnabled(true);
    }
}

void ImportBox::typeSelected()
{
    if( loadRB->isChecked())        
    {
        resetNoneRB->setEnabled(false);
        resetAllRB->setEnabled(false);
        resetAskRB->setEnabled(false);
        GString txt = "CAUTION!\n\nUsing LOAD can seriously damage the target table (and all its dependent tables).";
        txt += "\nUse this only if you really, really know what you are doing. ";
        txt += "\n\nAlso, when using LOAD, LOBs and XML data can only be read from the server side (!)";
        msg(txt);
        insRB->setEnabled(true);
        replRB->setEnabled(true);
        updRB->setEnabled(false);
        repCrRB->setEnabled(false);
        crtRB->setEnabled(false);
        if( updRB->isChecked() || repCrRB->isChecked() || crtRB->isChecked() )
        {
            msg("'Load' does not support UPDATE/CREATE/REPL.CREATE ");
            insRB->setChecked(true);
        }
    }
    else
    {
        resetNoneRB->setEnabled(true);
        resetAllRB->setEnabled(true);
        resetAskRB->setEnabled(true);
        insRB->setEnabled(true);
        replRB->setEnabled(true);
        updRB->setEnabled(true);
        repCrRB->setEnabled(true);
        crtRB->setEnabled(true);
    }
}

bool ImportBox::isHADRorLogArchMeth()
{
    if( m_pDSQL->getDBType() != DB2 ) return false;

    m_pDSQL->initAll("select * from SYSIBMADM.SNAPHADR");
    if( m_pDSQL->numberOfRows() ) return 1;
    GString res;
    DBAPIPlugin * pAPI = new DBAPIPlugin(m_pDSQL->getDBTypeName());
    pAPI->setGDebug(m_pGDeb);
    CON_SET conSet;
    m_pDSQL->currentConnectionValues(&conSet);

    //822 is SQLF_DBTN_LOGARCHMETH1 in sqlutil.h
    res = pAPI->getDbCfgForToken(conSet.Host, conSet.UID, conSet.PWD, conSet.DB, 822, 0);
    delete pAPI;
    if( res.upperCase() != "OFF" && res.length() ) return 1;
    return 0;
}

void ImportBox::optionsClicked()
{
    createExpImpOptions();
    m_pExpImpOptions->exec();
}

GString ImportBox::newCodePage(GString path)
{
#ifdef MAKE_VC
        if( Helper::fileIsUtf8(path) ) return "1208";
#else
        if( !Helper::fileIsUtf8(path) )return "1252";
#endif
    return "";
}

void ImportBox::callImport(GString fullPath)
{
    GString file;
    /*
    GString codePage = newCodePage(path);
    if( delRB->isChecked() && codePage.length())
    {
        getCodepage * foo = new getCodepage(this);
        foo->setValue(codePage);
        foo->exec();
        m_pExpImpOptions->setFieldValue(expImpOptions::TYP_DEL, "codepage=", foo->getValue());
    }
    */
    if( repCrRB->isChecked() )
    {
        if( QMessageBox::question(this, "PMF Import", "WARNING:\nThis will (Re-)create the table, existing data will be lost. Continue?", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes ) return;
    }
    if( replRB->isChecked() )
    {
        if( QMessageBox::question(this, "PMF Import", "WARNING:\nExisting data will be lost. Continue?", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes ) return;
    }
    GFile aFile(fullPath, GF_READONLY);
    if( !aFile.initOK() )
    {
        msg("Invalid path / filename.");
        return;
    }
    fullPath = fullPath.translate('\\', '/');
    if(  !fullPath.length() || fullPath.occurrencesOf("/") == 0 )
    {
        msg("Specify path and filename.");
        return;
    }

    file = fullPath.subString(fullPath.lastIndexOf('/')+1, fullPath.length()).strip();
    if( !file.strip().length() )
    {
        msg("Filename missing or invalid. ");
        return;
    }
    int i, found = -1;
    if( !crtRB->isChecked() )
    {
        for( i=0; i<tableLB->count(); ++i)
        {
            if( tableLB->item(i)->isSelected() )
            {
                found = i;
                break;
            }
        }
    }
    if( crtRB->isChecked() && GString(tableNameLE->text()).length() == 0 )
    {
        msg("Specify table to create or select table");
        tableNameLE->setFocus();
        return;
    }

    GString tableName;
    if( found >= 0 ) tableName = currentSchema+"."+GString(tableLB->item(found)->text());        

    //Create table: tableName from LineEdit
    if( crtRB->isChecked() ) tableName = GString(tableNameLE->text());
    else if ( repCrRB->isChecked() )
    {
        if( tableNameLE->text().length() ) tableName = GString(tableNameLE->text());
    }

    if( !tableName.length() )
    {
        msg("No table name selected or specified.\nTo import, select existing table from the right; to create, enter table name below.");
        return;
    }

    if( QMessageBox::question(this, "PMF", "Importing into\n\n"+tableName+"\n\nContinue?", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes ) return;


    //Rather do this via Menu->Reset GENERATED if required
    if( resetGeneratedCols() ) return;

    timer->start( 200 );
    aThread = new ImportThread;
    tb = new ThreadBox( this, "Please wait...", "Importing", m_pDSQL->getDBTypeName() );
    tb->setThread(aThread);
    aThread->setOwner( this );
    aThread->setBox(tb);
    aThread->start();
    GStuff::dormez(300);

    tb->exec();

    /******
while( aThread->isAlive() )
{
GStuff::dormez(100);
printf("alive\n");
}
   //tb = NULL;
   //aThread->setDone();
*****/
    //tm("Done.");
}

void ImportBox::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Escape:
            this->close();


    }
}

void ImportBox::closeEvent(QCloseEvent * event)
{
    QFile f(m_strLogFile);
    if( f.size() > 0)
    {
        //if( QMessageBox::question(this, "Import", "Delete Logfile?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )
        {
            remove(m_strLogFile);
        }
    }
    event->accept();
}


void ImportBox::displayLog()
{

    if( loadRB->isChecked() && !m_sqlErc )
    {
        runLoadChecks();
    }
    errorLB->update();

    GString errTxt;
    //if( m_sqlErc )

    QFile f(m_strLogFile);
    if( f.size() > 1000000)
    {
        GString msg;
        if( m_sqlErc ) msg = "There were errors.\n";
        else msg = "Import was successful.\n";
        msg += "Logfile is in "+m_strLogFile+" and it is too big to show ("+GString((unsigned long)f.size()/1000000)+"MB), please check it manually.";
        msg += "\n\nHint: Closing this dialog will remove the logfile.";
        errorLB->setText(msg);
        return;
    }
    errTxt += "Please wait, reading LogFile...\n";
    errTxt += "   ****  LogFile: "+m_strLogFile+"  ****\n";
    errTxt += "ErrorCode: "+GString(m_sqlErc)+"\n";
    errTxt += m_sqlErrTxt+"\n";

    errorLB->blockSignals( true );
    if( f.open(QIODevice::ReadOnly) )
    {
        QTextStream t( &f );
        errTxt += t.readAll();
        f.close();
    }
    else errTxt += "   No LogFile was created.\n";
    errorLB->blockSignals( false );

    GString hints = "Some general hints if Import or Load failed:\n\n";
    hints += "- Try to import from a LOCAL drive\n";
    hints += "- Files over 4GB will usually fail.\n";
    hints += "- LOAD: LOBs and XMLs must be on a local drive on the SERVER!\n";
    hints += "  LOAD with LOBs and XMLs from a client will NOT work.";
    errorLB->setText(m_importMsg+"\n"+ errTxt);
    errorLB->verticalScrollBar()->setValue(errorLB->verticalScrollBar()->maximum());
    //else errorLB->addItem("   No error was returned.");
    if( m_sqlErc )
    {
        QMessageBox::information(this, "pmf", hints);
    }
}
void ImportBox::timerEvent()
{
    if( !aThread ) return;
    if( !aThread->isAlive() )
    {
        if( tb )
        {
            tb->close();
        }
        displayLog();
        timer->stop();
    }
}

void ImportBox::ImportThread::run()
{
   myImport->startImport();
}

void ImportBox::OKClicked()
{
    if( m_impFileSeq.numberOfElements() == 0  )
    {
        msg("Select file(s) to import");
        return;
    }
    if( m_pExpImpOptions == NULL)
    {
        createExpImpOptions();
    }

    if( isHADRorLogArchMeth() && loadRB->isChecked() )
    {
        int err = 0;
        if( m_pExpImpOptions != NULL )
        {
            if( m_pExpImpOptions->getFieldValue(ExpImpOptions::TYP_LOAD, "to ").length() == 0 || m_pExpImpOptions->getCheckBoxValue(ExpImpOptions::TYP_LOAD, "copy yes") == 0 )
            {
                err = 1;
            }
        }
        else err = 1;
        if( err )
        {
            Helper::msgBox(this, "pmf", "This appears to be a HADR system or LOGARCHMETH1 is enabled.\nOpen 'Options', check 'COPY YES' and set a path.");
            return;
        }
    }
    remove(m_strLogFile);
    //Do not call setComboBoxesFromFile(), this would override user's choice of target table
    if( m_impFileSeq.numberOfElements() == 1 )
    {
        callImport(m_impFileSeq.elementAtPosition(1));
    }
    else
    {
        for(int i = 1; i <= m_impFileSeq.numberOfElements(); ++i)
        {
            GString file = m_impFileSeq.elementAtPosition(i);
            setComboBoxesFromFile(file);
            fileNameDropZone->setText(file);
            callImport(file);
        }
        fileNameDropZone->clearFileList();
        m_impFileSeq.removeAll();
    }
    QSettings settings(_CFG_DIR, "pmf6");
    QString path = GStuff::pathFromFullPath(fileNameDropZone->text());
    settings.setValue("importPath", path);
    setImportOptionsEnabled(true);
}

void ImportBox::createExpImpOptions()
{    
    CON_SET conSet;
    m_pDSQL->currentConnectionValues(&conSet);
    if( m_pExpImpOptions ) delete m_pExpImpOptions;
    if( importRB->isChecked() )
    {
        m_pExpImpOptions = new ExpImpOptions(this, ExpImpOptions::MODE_IMPORT, &conSet);
    }
    else if( loadRB->isChecked() )
    {
        if( isHADRorLogArchMeth() )m_pExpImpOptions = new ExpImpOptions(this, ExpImpOptions::MODE_LOAD_HADR, &conSet);
        else m_pExpImpOptions = new ExpImpOptions(this, ExpImpOptions::MODE_LOAD, &conSet);
    }
}

GString ImportBox::getTargetTable()
{
    signed int found = -1;
    if( !crtRB->isChecked() )
    {
        for( int i = 0; i < tableLB->count(); ++i )
        {
            if( tableLB->item(i)->isSelected() )
            {
                found = i;
                break;
            }
        }
    }
    //tableName from ListBox selection
    GString tableName;
    if( found >= 0 && m_pDSQL->getDBType() == MARIADB ) tableName = currentSchema+"."+GString(tableLB->item(found)->text());
    else if( found >= 0 ) tableName = "\""+currentSchema+"\".\""+GString(tableLB->item(found)->text())+"\"";
    return tableName;
}

void ImportBox::startImport()
{    
    if( m_pDSQL->getDBType() == POSTGRES ) startPGSQLImport();
    else if( m_pDSQL->getDBType() == DB2 && csvRB->isChecked() ) startDB2CsvImport();
    else startDB2Import();
}

void ImportBox::startPGSQLImport()
{
    GString tableName = getTargetTable();
    //tableName = GStuff::decorateTabName(tableNameLE->text());

    PmfTable pmfTable =  PmfTable(m_pDSQL, tableName);
    if( !tableName.length() ) return;
    GString path, file, fullPath;
    fullPath = fileNameDropZone->text();
    file = fullPath;

    #ifdef MAKE_VC
    fullPath = fullPath.translate('/', '\\');
    path = path.translate('/', '\\');
    file = file.translate('/', '\\');
    if( fullPath.occurrencesOf('\\') > 0 ) path = fullPath.subString(1,fullPath.lastIndexOf('\\'));
    else path = "";
    #else
    if( fullPath.occurrencesOf('/') > 0 ) path = fullPath.subString(1,fullPath.lastIndexOf('/'));
    else path = "";
    #endif

    *m_gstrPrevDir = path;

    GFile f(file);
    GString line;
    GString delim = m_pExpImpOptions->getFieldValue(ExpImpOptions::TYP_CSV, "Delimiter");
    int commitCount = m_pExpImpOptions->getFieldValue(ExpImpOptions::TYP_CSV, "CommitCount").asInt();
    int byteaAsFile = m_pExpImpOptions->getCheckBoxValue(ExpImpOptions::TYP_CSV, "ByteaFiles");
    int xmlAsFile = m_pExpImpOptions->getCheckBoxValue(ExpImpOptions::TYP_CSV, "XmlFiles");
    int header = m_pExpImpOptions->getCheckBoxValue(ExpImpOptions::TYP_CSV, "Header");

    if( !delim.strip().length() ) delim ="|";
    if( f.lines() == 0 ) return;
    remove(m_strLogFile);
    if( f.lines() == 0 )
    {
        writeLog("No lines to import in file "+file);
        return;
    }

    GString cmd, err;
    line = f.initLineCrs();
    int count = 0;
    int fullCount = 0;
    if( header )
    {
        if( f.lines() == 1 )
        {
            writeLog("No lines to import in file "+file);
            return;
        }
        f.nextLineCrs();
    }
    if( runCsvChecks(&pmfTable, line, delim) == 2 )f.nextLineCrs();
    if( runCsvChecks(&pmfTable, line, delim) == 1 )
    {
        writeLog("Cannot import: Number of columns in file do not match columns");
        return;
    }
    if( commitCount > 0 ) err = m_pDSQL->initAll("BEGIN");
    while( 1 )
    {
        line = f.lineAtCrs().stripTrailing('\n');
        fullCount++;
        cmd = createPgsqlCsvInsertCmd(&pmfTable, line, delim, byteaAsFile, xmlAsFile);
        if( cmd == ERR_COL_NUMBER )
        {
            writeLog("Line #"+GString(fullCount)+" cannot be imported: Wrong number of columns");
            if( !f.nextLineCrs() ) break;
            continue;
        }
        err = m_pDSQL->initAll(cmd);
        if( err.length() ) writeLog("Line #"+GString(fullCount)+" cannot be imported: "+err);
        count++;

        if( count >= commitCount && commitCount > 0 )
        {
            count = 0;
            err = m_pDSQL->initAll("COMMIT");
            err = m_pDSQL->initAll("BEGIN");
        }
        if( !f.nextLineCrs() ) break;        
    }
    if( commitCount > 0 ) err = m_pDSQL->initAll("COMMIT");
}

void ImportBox::startDB2CsvImport()
{
    GString tableName = getTargetTable();
    //tableName = GStuff::decorateTabName(tableNameLE->text());

    PmfTable pmfTable =  PmfTable(m_pDSQL, tableName);
    if( !tableName.length() ) return;
    GString path, file, fullPath;
    fullPath = fileNameDropZone->text();
    file = fullPath;

    #ifdef MAKE_VC
    fullPath = fullPath.translate('/', '\\');
    path = path.translate('/', '\\');
    file = file.translate('/', '\\');
    if( fullPath.occurrencesOf('\\') > 0 ) path = fullPath.subString(1,fullPath.lastIndexOf('\\'));
    else path = "";
    #else
    if( fullPath.occurrencesOf('/') > 0 ) path = fullPath.subString(1,fullPath.lastIndexOf('/'));
    else path = "";
    #endif

    *m_gstrPrevDir = path;

    GFile f(file);
    GString line;
    GString delim = m_pExpImpOptions->getFieldValue(ExpImpOptions::TYP_CSV, "Delimiter");
    int commitCount = m_pExpImpOptions->getFieldValue(ExpImpOptions::TYP_CSV, "CommitCount").asInt();
    int byteaAsFile = m_pExpImpOptions->getCheckBoxValue(ExpImpOptions::TYP_CSV, "ByteaFiles");
    int xmlAsFile = m_pExpImpOptions->getCheckBoxValue(ExpImpOptions::TYP_CSV, "XmlFiles");
    int header = m_pExpImpOptions->getCheckBoxValue(ExpImpOptions::TYP_CSV, "Header");

    if( !delim.strip().length() ) delim ="|";
    if( f.lines() == 0 ) return;
    remove(m_strLogFile);
    if( f.lines() == 0 )
    {
        writeLog("No lines to import in file "+file);
        return;
    }

    GString cmd, err;
    line = f.initLineCrs();
    int count = 0;
    int fullCount = 0;
    if( header )
    {
        if( f.lines() == 1 )
        {
            writeLog("No lines to import in file "+file);
            return;
        }
        f.nextLineCrs();
    }
    if( runCsvChecks(&pmfTable, line, delim) == 2 )f.nextLineCrs();
    if( runCsvChecks(&pmfTable, line, delim) == 1 )
    {
        writeLog("Cannot import: Number of columns in file do not match columns");
        return;
    }
    while( 1 )
    {
        line = f.lineAtCrs().stripTrailing('\n');
        fullCount++;
        cmd = createDb2CsvInsertCmd(&pmfTable, line, delim, byteaAsFile, xmlAsFile);
        if( cmd == ERR_COL_NUMBER )
        {
            writeLog("Line #"+GString(fullCount)+" cannot be imported: Wrong number of columns");
            if( !f.nextLineCrs() ) break;
            continue;
        }
        if( err.length() ) writeLog("Line #"+GString(fullCount)+" cannot be imported: "+err);
        count++;

        if( count >= commitCount && commitCount > 0 )
        {
            count = 0;
            m_pDSQL->commit();
        }
        if( !f.nextLineCrs() ) break;
    }
}

GString ImportBox::createDb2CsvInsertCmd(PmfTable * pmfTable, GString line, GString delim, int byteaAsFile, int xmlAsFile)
{
    GString data;
    GSeq <GString> lobFileSeq;
    GSeq <long> lobTypeSeq;

    GString cmd = "INSERT INTO "+pmfTable->quotedName()+" (";
    for( int i = 1; i<= pmfTable->columnCount(); ++i )
    {
        GString colName = pmfTable->column(i)->quotedName();
        if( pmfTable->column(i)->identity().length()  ) continue;
        cmd += colName + GString(",");
    }
    cmd = cmd.stripTrailing(",") + ") VALUES (";
    GSeq <GString> elmts = line.split(delim);

    if( pmfTable->columnCount() != elmts.numberOfElements()) return ERR_COL_NUMBER;

    for(int i = 1; i <= elmts.numberOfElements(); ++i )
    {
        if( pmfTable->column(i)->identity().length()  ) continue;
        PmfColumn *pCol = pmfTable->column(i);
        GString x = elmts.elementAtPosition(i);
        if( elmts.elementAtPosition(i) != "NULL" && pCol->colType() == "BLOB" && byteaAsFile )
        {
            cmd += GString("?,");
            lobFileSeq.add(elmts.elementAtPosition(i));
            lobTypeSeq.add(404);
        }
        else if( elmts.elementAtPosition(i) != "NULL" && pCol->colType() == "XML" && xmlAsFile)
        {
            cmd += GString("?,");
            lobFileSeq.add(elmts.elementAtPosition(i));
            lobTypeSeq.add(988);
        }
        else cmd += elmts.elementAtPosition(i) + GString(",");
    }
    cmd = cmd.stripTrailing(",") + ")";
    int erc = m_pDSQL->uploadBlob(cmd, &lobFileSeq, &lobTypeSeq);
    printf("CMD: %s\n", (char*) cmd);
    if( erc ) return m_pDSQL->sqlError();
    return "";

}

int ImportBox::runCsvChecks(PmfTable * pmfTable, GString line, GString delim)
{
    line = line.strip('\n');
    GSeq <GString> elmts = line.split(delim);
    if( elmts.numberOfElements() != pmfTable->columnCount() ) return 1;
    GString elmString, colString;
    for( int i = 1; i<= pmfTable->columnCount(); ++i )
    {
        elmString += elmts.elementAtPosition(i).upperCase();
        colString += GString(pmfTable->column(i)->colName()).upperCase();
    }
    if( elmString == colString ) return 2;
    return 0;
}

GString ImportBox::createPgsqlCsvInsertCmd(PmfTable * pmfTable, GString line, GString delim, int byteaAsFile, int xmlAsFile)
{
    GString data;
    GString cmd = "INSERT INTO "+pmfTable->quotedName()+" (";
    for( int i = 1; i<= pmfTable->columnCount(); ++i )
    {        
        GString colName = pmfTable->column(i)->quotedName();
        if( pmfTable->column(i)->identity().length()  ) continue;
        cmd += colName + GString(",");
    }
    cmd = cmd.stripTrailing(",") + ") VALUES (";
    GSeq <GString> elmts = line.split(delim);

    if( pmfTable->columnCount() != elmts.numberOfElements()) return ERR_COL_NUMBER;

    for(int i = 1; i <= elmts.numberOfElements(); ++i )
    {
        if( pmfTable->column(i)->identity().length()  ) continue;
        PmfColumn *pCol = pmfTable->column(i);
        if( elmts.elementAtPosition(i) != "NULL" &&
               (  (pCol->colType() == "bytea"  && byteaAsFile) || (pCol->colType() == "xml"  && xmlAsFile) ) )
        {
            loadFileIntoBuf(elmts.elementAtPosition(i), &data);
            cmd += data + GString(",");
        }
        else cmd += elmts.elementAtPosition(i) + GString(",");
    }
    cmd = cmd.stripTrailing(",") + ")";
    return cmd;
}


int ImportBox::loadFileIntoBuf(GString fileName, GString* data)
{
    FILE * f;
    f = fopen(fileName, "rb");
    if( f != NULL )
    {
        char* fb;
        int sz;
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        fb = new char[sz+1];
        fread(fb, 1, sz, f);
        fclose(f);
        fb[sz] = '\0';
        *data = GString(fb, sz).stripTrailing('\n');
        delete fb;
        return 0;
    }
    return 1;
}

int ImportBox::loadFileIntoBuf(GString fileName, char* fileBuf, int *size)
{
    FILE * f;
    f = fopen(fileName, "rb");
    if( f != NULL )
    {
        char* fb;
        int sz;
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        fb = new char[sz+1];
        fread(fb, 1, sz, f);
        fclose(f);
        fb[sz] = '\0';
        *size = sz;
        fileBuf = fb;
        return 0;
    }
    return 1;
}


void ImportBox::writeLog(GString msg)
{
    GFile logFile(m_strLogFile);
    logFile.addLine(msg);
}

void ImportBox::startDB2Import()
{

    GString format;
    GString action, modifier, copyTarget;

    if( m_pExpImpOptions != NULL)
    {
        if( importRB->isChecked() )  modifier = m_pExpImpOptions->createModifiedString(ExpImpOptions::TYP_ALL);
        else if( loadRB->isChecked() )  modifier = m_pExpImpOptions->createModifiedString(ExpImpOptions::TYP_LOAD);
        if( m_pExpImpOptions->getCheckBoxValue(ExpImpOptions::TYP_LOAD, "copy yes") )
        {
            copyTarget = m_pExpImpOptions->getFieldValue(ExpImpOptions::TYP_LOAD, "to ");
        }
    }

    if( delRB->isChecked() )
    {
        if( m_pExpImpOptions != NULL) modifier += " "+ m_pExpImpOptions->createModifiedString(ExpImpOptions::TYP_DEL);
        format = "DEL" ;
    }
    else if( ixfRB->isChecked() )
    {
        if( m_pExpImpOptions != NULL) modifier += " "+ m_pExpImpOptions->createModifiedString(ExpImpOptions::TYP_IXF);
        format = "IXF";
    }
    if(wsfRB->isChecked() ) format = "WSF";

    if( insRB->isChecked() ) action = "INSERT INTO ";
    if( updRB->isChecked() ) action = "INSERT_UPDATE INTO ";
    if( crtRB->isChecked() ) action = "CREATE INTO ";
    if( replRB->isChecked() ) action = "REPLACE INTO ";
    if( repCrRB->isChecked() ) action = "REPLACE_CREATE INTO ";

    //Some of these checks were done above, but this runs in a thread.
    GString tableName = getTargetTable();

    //Create table: tableName from LineEdit
    if( crtRB->isChecked() || repCrRB->isChecked() )
    {
        tableName = GStuff::decorateTabName(tableNameLE->text());
    }

    if( !tableName.length() ) return;
    action += tableName;

    GString path, file, fullPath;
    fullPath = fileNameDropZone->text();
    file = fullPath;

    #ifdef MAKE_VC
    fullPath = fullPath.translate('/', '\\');
    path = path.translate('/', '\\');
    file = file.translate('/', '\\');
    if( fullPath.occurrencesOf('\\') > 0 ) path = fullPath.subString(1,fullPath.lastIndexOf('\\'));
    else path = "";
    #else
    if( fullPath.occurrencesOf('/') > 0 ) path = fullPath.subString(1,fullPath.lastIndexOf('/'));
    else path = "";
    #endif

    *m_gstrPrevDir = path;
    //On error: Try again w/out "lobsinfile"


    if( m_pDSQL->getDBType() == MARIADB )
    {
        GString cmd = "LOAD DATA INFILE '"+file+"' INTO TABLE "+tableName;
        m_sqlErrTxt = m_pDSQL->initAll(cmd);
        return;
    }



    DBAPIPlugin * pAPI = new DBAPIPlugin(m_pDSQL->getDBTypeName());
    m_importMsg = "";
    if( !pAPI )
    {
        m_importMsg += "Could not load DBAPiPlugin (should be in the 'plugin' subdirectory)\n";
        return;
    }
    pAPI->setGDebug(m_pGDeb);
    m_sqlErc = 0;
    m_sqlErrTxt = "";

    //Last param is modifier
    //0: None
    //1: MODIFIED BY LOBSINFILE
    //2: MODIFIED BY IDENTITYIGNORE
    //3: MODIFIED BY IDENTITYIGNORE AND LOBSINFILE
    //4: MODIFIED BY IDENTITYOVERRIDE
    GSeq<GString> pathSeq;

    pathSeq.add(path.strip());
    if( loadRB->isChecked() )
    {
        m_importMsg += "----------\n";
        m_importMsg += " Modifier: "+modifier+"\n";
        m_importMsg += " CopyTarget: "+copyTarget+"\n";
        m_importMsg += "----------\n";
        m_sqlErc = pAPI->loadFromFileNew(file, &pathSeq, format, action, m_strLogFile, modifier, copyTarget);
        if( m_sqlErc) m_sqlErrTxt = pAPI->SQLError();

        if( m_sqlErc && m_sqlErc != 3107) m_sqlErc = pAPI->loadFromFileNew(file, &pathSeq, format, action, m_strLogFile, "lobsinfile identityignore", copyTarget);
        if( m_sqlErc && m_sqlErc != 3107) m_sqlErc = pAPI->loadFromFileNew(file, &pathSeq, format, action, m_strLogFile, "identityignore", copyTarget);
        if( m_sqlErc && m_sqlErc != 3107) m_sqlErc = pAPI->loadFromFileNew(file, &pathSeq, format, action, m_strLogFile, "lobsinfile", copyTarget);
        if( m_sqlErc && m_sqlErc != 3107) m_sqlErc = pAPI->loadFromFileNew(file, &pathSeq, format, action, m_strLogFile, "FORCECREATE", copyTarget);
        if( !m_sqlErc )
        {
            m_importMsg += "--- LOAD RETURNED NO ERROR.\n";
        }

    }
    else
    {
//        m_sqlErc = pAPI->importTable(file, &pathSeq, format, action, m_strLogFile, modifier);
//        if( m_sqlErc && m_sqlErc != 3107 ) m_sqlErc = pAPI->importTable(file, &pathSeq, format, action, m_strLogFile, modifier);
//        if( m_sqlErc && m_sqlErc != 3107 ) m_sqlErc = pAPI->importTable(file, &pathSeq, format, action, m_strLogFile, modifier);
//        if( m_sqlErc && m_sqlErc != 3107 )
//        {
//            m_sqlErrTxt = pAPI->SQLError();
//            errorLB->addItem("Trying again without LOBs and FORCECREATE...");
//            m_sqlErc = pAPI->importTable(file, &pathSeq, format, action, m_strLogFile, modifier);
//        }
        remove(m_strLogFile);
        m_sqlErc = pAPI->importTableNew(file, &pathSeq, format, action, m_strLogFile, "lobsinfile identityignore "+modifier);
        if( m_sqlErc && m_sqlErc != 3107 && m_sqlErc != -3005) m_sqlErc = pAPI->importTableNew(file, &pathSeq, format, action, m_strLogFile, "identityignore "+modifier);
        if( m_sqlErc && m_sqlErc != 3107 && m_sqlErc != -3005) m_sqlErc = pAPI->importTableNew(file, &pathSeq, format, action, m_strLogFile, "lobsinfile "+modifier);
        if( m_sqlErc && m_sqlErc != 3107 && m_sqlErc != -3005)
        {
            m_sqlErrTxt = pAPI->SQLError();
            m_importMsg += "Trying again without LOBs and FORCECREATE...\n";
            m_sqlErc = pAPI->importTableNew(file, &pathSeq, format, action, m_strLogFile, "FORCECREATE "+modifier);
        }
    }
    delete pAPI;

}

void ImportBox::runLoadChecks()
{
    PmfTable pmfTable(m_pDSQL, currentTable);
    GString property, outMsg, cmd;
    GString schema = pmfTable.tabSchema();
    GString table  = pmfTable.tabName();
    m_pDSQL->initAll("select STATUS, ACCESS_MODE, PROPERTY from SYSCAT.TABLES WHERE TabSchema='"+schema+"' and TabName='"+table+"'");
    if( m_pDSQL->numberOfRows() == 1 )
    {
        if( m_pDSQL->rowElement(1, "STATUS").strip().upperCase() != "'N'" ||  m_pDSQL->rowElement(1, "ACCESS_MODE").strip().upperCase() != "'F'")
        {
            outMsg = "Table is in integrity pending state.\n";
            outMsg += "Would you like to run\nSET INTEGRITY FOR <table> IMMEDIATE CHECKED?";
            cmd = "SET INTEGRITY FOR "+currentTable+" IMMEDIATE CHECKED";
            property = m_pDSQL->rowElement(1, "PROPERTY").strip().strip("'").upperCase();
            if(property.length() > 0)
            {
                if( property[1] == 'Y')
                {
                    outMsg = "This appears to be a user maintained materialized query table.\n";
                    outMsg += "Would you like to run\nSET INTEGRITY FOR <table> ALL IMMEDIATE UNCHECKED?";
                    cmd = "SET INTEGRITY FOR "+currentTable+" ALL IMMEDIATE UNCHECKED";
                }
            }
            if( QMessageBox::question(this, "PMF", outMsg, QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes ) return;
            GString err = m_pDSQL->initAll(cmd);
            if( err.length() )m_importMsg += "!!! INTEGRITY CHECK failed: "+err+"\n";
            else m_importMsg += "--- INTEGRITY CHECK returned no error.\n";
        }
    }
    for( int i = 1; i <= pmfTable.columnCount(); ++i )
    {
        int max = 0;
        if( GString(pmfTable.column(i)->misc()).occurrencesOf("GENERATED ALWAYS") )
        {
            cmd = "select max(" +pmfTable.column(i)->colName()+") from "+currentTable;
            GString err = m_pDSQL->initAll(cmd);
            if( !err.length() )
            {
                max = m_pDSQL->rowElement(1, 1).strip("'").asInt()+1;
                outMsg = "Column "+pmfTable.column(i)->colName()+" is GENERATED ALWAYS. Do you want to restart the counter with current MAX?";
                if( QMessageBox::question(this, "PMF", outMsg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )
                {
                    cmd = "ALTER TABLE "+currentTable+" ALTER COLUMN " +pmfTable.column(i)->colName()+" RESTART WITH "+GString(max);
                    err = m_pDSQL->initAll(cmd);
                    if(err.length())msg(err);
                }
            }
        }
    }
}


void ImportBox::CancelClicked()
{
    fileNameDropZone->clearFileList();
    remove(m_strLogFile);
    close();
}

void ImportBox::getFileClicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(this, "Select file to import", *m_gstrPrevDir);

    if( filenames.isEmpty() ) return;
    else if (filenames.count() == 1 ) fileNameDropZone->setText(filenames.at(0));
    else fileNameDropZone->setText("["+GString(filenames.count()+" files]"));
    m_impFileSeq.removeAll();
    for( int i = 0; i < filenames.count(); ++i )
    {
        m_impFileSeq.add(GString(filenames.at(i)));
    }
    GString ret = setFilesToImport(m_impFileSeq);
    if( ret != "OK" )
    {
        fileNameDropZone->clearFileList();
        if(ret.length()) msg(ret);
    }
}

short ImportBox::fillLB(GSeq <GString> * tableNameSeq)
{

    for( unsigned i=1; i<=tableNameSeq->numberOfElements(); ++i)
    {
        tableLB->addItem(tableNameSeq->elementAtPosition(i));
    }
    return 0;
}

void ImportBox::deleteErrLog()
{
    errorLB->clear();
    remove(m_strLogFile);
//    QFile f(m_strLogFile);
//    f.remove();

    //if( !f.remove() ) errorLB->addItem("<Could Not Remove File "+m_strLogFile+". Sorry.>");

}
void ImportBox::setSchema(GString schema)
{
    iSchema = schema;
}
void ImportBox::fillLB(GString schema, GString table)
{
    GString name;
    m_pDSQL->getTables(schema);
    tableLB->clear();
    QListWidgetItem *pItem = NULL;
    int pos = -1;
    for( unsigned int i=1; i<=m_pDSQL->numberOfRows(); ++i)
    {
        name   = m_pDSQL->rowElement(i,1).strip().strip("'").strip();
        new QListWidgetItem(name, tableLB);
        if( "\""+schema+"\".\""+name+"\"" == table )
        {
            pos = i-1;
            pItem = tableLB->item(pos);
            pItem->setSelected(true);
            tableLB->setCurrentRow(pos);
        }
    }
}
void ImportBox::schemaSelected(int index)
{
    fillLB(schemaCB->itemText(index));
    currentSchema = schemaCB->itemText(index);
}

void ImportBox::setButtons()
{
    QFont font  = this->font();
    font.setBold(false);
    delRB->setFont(font);
    wsfRB->setFont(font);
    ixfRB->setFont(font);

    delRB->setText("DEL Format");
    wsfRB->setText("WSF Format");
    ixfRB->setText("IXF Format");
}


IMPORT_TYPE ImportBox::importTypeFromFile(GString file)
{
    setButtons();
    FILE *f;
    char *buffer;
    int n;
    unsigned long size;
    IMPORT_TYPE type = IMP_FILE_UNKNOWN;

    f = fopen(file, "rb");
    if( !f ) return type;

    //Get file size
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    //Read max 1000 bytes from file
    if( size > 1000 ) size = 1000;
    buffer=(char *)malloc(size+1);
    n = fread(buffer, size, sizeof(char), f);


    if( GString(buffer, 20).occurrencesOf("IXF")) type = IMP_FILE_IXF;
    else if( GString(buffer, 8, GString::HEX) == GString("0000020004040700") ) type = IMP_FILE_WSF;
    else if ( memchr(buffer, '\0', size) == NULL ) type = IMP_FILE_DEL;    
    fclose(f);
    free(buffer);
    return type;
}

void ImportBox::setImportTypeFromFileType(GString file)
{
    setButtons();
    QFont font = this->font();
    font.setBold(true);

    if(m_pDSQL->getDBType() == POSTGRES)
    {
        csvRB->setChecked(true);
        csvRB->setFont(font);
    }
    else if( importTypeFromFile(file) == IMP_FILE_IXF )
    {
        ixfRB->setChecked(true);
        ixfRB->setText("IXF Format (guessed)");
        ixfRB->setFont(font);
    }
    else if( importTypeFromFile(file) == IMP_FILE_WSF )
    {
        wsfRB->setChecked(true);
        wsfRB->setText("WSF Format (guessed)");
        wsfRB->setFont(font);
    }
    else if ( importTypeFromFile(file) == IMP_FILE_DEL )
    {
        delRB->setChecked(true);
        delRB->setText("DEL Format (guessed)");
        delRB->setFont(font);
    }
}



int ImportBox::checkForSameImportType(GSeq<GString> *fileSeq)
{
    if( fileSeq->numberOfElements() > 1 )
    {
        IMPORT_TYPE type = importTypeFromFile(fileSeq->elementAtPosition(1));
        for( int i = 2; i <= fileSeq->numberOfElements(); ++i )
        {
            if( importTypeFromFile(fileSeq->elementAtPosition(1)) != type )
            {
                return 1;
            }
        }
    }
    return 0;
}

int ImportBox::showMultiImport(GSeq<GString> *fileSeq)
{
    if( fileSeq->numberOfElements() <= 1 ) return QDialog::Accepted;
    MultiImport * foo = new MultiImport(m_pGDeb, m_pDSQL, this);
    GString fileName;
    for( int i = 1; i <= fileSeq->numberOfElements(); ++i )
    {
        fileName = Helper::fileNameFromPath(fileSeq->elementAtPosition(i));
        foo->createRow(fileName, schemaFromFile(fileName), tableFromFile(fileName));
    }    
    return foo->exec();
}

GString ImportBox::setFilesToImport(GSeq<GString> fileSeq)
{
    m_impFileSeq.removeAll();
    if( fileSeq.numberOfElements() == 0 )
    {
        handleImportFiles();
        return "OK";
    }
    if( checkForSameImportType(&fileSeq) )
    {
        fileNameDropZone->clearFileList();
        return "Files are not of same type (IXF, DEL, WSF)";
    }
    for( int i = 2; i <= fileSeq.numberOfElements(); ++i )
    {
        if( !canMapToTable(fileSeq.elementAtPosition(i)) )
        {
            fileNameDropZone->clearFileList();
            GString m = "Not all files can be mapped to tables. The filenames must be in the form of\n\n";
            m += "[Schema].[Table] or\n[Schema].[Table].[Extension]\n\n";
            m += "PMF will then import the files into the corresponding tables.";
            m += "\n\nHint: When importing LOBs, do *not* select the files containing the actual LOBs.";
            return m;
        }
    }

    m_impFileSeq = fileSeq;
    handleImportFiles();
    int rc = showMultiImport(&fileSeq);
    if( rc != QDialog::Accepted )
    {
        m_impFileSeq.removeAll();
        setImportOptionsEnabled(true);
        return "";
    }
    return "OK";

    //Todo:
    //Check types
    //guessSchemaAndTableFromFile
    //Create struct
//    fileNameLE->setText(file);
//    setImportTypeFromFileType();
//    guessSchemaAndTableFromFile();
}

void ImportBox::setImportOptionsEnabled(bool enabled)
{
    crtRB->setEnabled(enabled);
    repCrRB->setEnabled(enabled);
    schemaCB->setEnabled(enabled);
    tableLB->setEnabled(enabled);
    if( !enabled ) insRB->setChecked(true);
}

void ImportBox::handleImportFiles()
{
    setImportOptionsEnabled(true);
    if( m_impFileSeq.numberOfElements() == 0 ) return;    
    else if (m_impFileSeq.numberOfElements() == 1 ) fileNameDropZone->setText(m_impFileSeq.elementAtPosition(1));
    else
    {
        fileNameDropZone->setText("["+GString(m_impFileSeq.numberOfElements())+" files to import]");
        setImportOptionsEnabled(false);
    }
    setImportTypeFromFileType(m_impFileSeq.elementAtPosition(1));
    setComboBoxesFromFile(m_impFileSeq.elementAtPosition(1));
}


int ImportBox::canMapToTable(GString input)
{
    GString schema = schemaFromFile(input);
    GString table = tableFromFile(input);
    GString cmd = "Select count(*) from syscat.tables where translate(tabschema)='"+schema.upperCase()+"' and translate(tabname)='"+table.upperCase()+"'";    
    m_pDSQL->initAll(cmd);

    if( m_pDSQL->rowElement(1,1).asInt() != 1 ) return 0;
    return 1;
}

GString ImportBox::tableFromFile(GString file)
{
    if( file.occurrencesOf(".") == 0 ) return  "";
    file = Helper::fileNameFromPath(file);
    GSeq<GString> seq = file.split('.');
    if( seq.numberOfElements() >= 2 ) return seq.elementAtPosition(2);
    return "";
}

GString ImportBox::typeFromFile(GString file)
{
    if( file.occurrencesOf(".") < 2 ) return  "";
    file = Helper::fileNameFromPath(file);
    GSeq<GString> seq = file.split('.');
    if( seq.numberOfElements() >= 3 ) return seq.elementAtPosition(3);
    return "";
}

GString ImportBox::schemaFromFile(GString file)
{
    if( file.occurrencesOf(".") == 0 ) return  "";
    file = Helper::fileNameFromPath(file);
    return file.subString(1, file.indexOf(".")-1);
}


void ImportBox::filesDropped()
{    
    GString ret = setFilesToImport(fileNameDropZone->fileList());
    if( ret != "OK" )
    {
        if( ret.length() ) msg(ret);
    }
}

void ImportBox::setComboBoxesFromFile(GString input)
{
    if( !input.length() ) return;
    GString schema = schemaFromFile(input);
    GString table = tableFromFile(input);
    GString cmd = "Select count(*) from syscat.tables where translate(tabschema)='"+schema.upperCase()+"' and translate(tabname)='"+table.upperCase()+"'";
    m_pDSQL->initAll(cmd);
    if( m_pDSQL->rowElement(1,1).asInt() != 1 )
    {
        GString type = typeFromFile(input).upperCase();
        if( type == "IXF" ) tableNameLE->setText(schema+"."+table);
        return;
    }

    //if( QMessageBox::question(this, "PMF Import", "Filename suggests target table "+schema+"."+table+"\nImport?", QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes ) return;


    int index = schemaCB->fill(m_pDSQL, schema, 0);
    if( index < 0 )
    {
        schemaCB->fill(m_pDSQL, currentSchema, m_iHideSysTables);
        index = schemaCB->fill(m_pDSQL, schema, 0);
    }
    if( index < 0 ) return;

    fillLB(schemaCB->itemText(index));
    currentSchema = schemaCB->itemText(index);
    QListWidgetItem *pItem;
    QList<QListWidgetItem*> items = tableLB->findItems(table, Qt::MatchCaseSensitive);
    if( items.count() ==  0 ) items = tableLB->findItems(table, Qt::MatchFixedString);
    if( items.count() == 1 )
    {
        pItem = items.at(0);
        tableLB->setCurrentItem(pItem);
        tableNameLE->setText(schema+"."+table);
    }
}

int ImportBox::hasGeneratedColumns()
{
    PmfTable pmfTable(m_pDSQL, currentTable);
    if( m_pDSQL->getDBType() != ODBCDB::DB2 && m_pDSQL->getDBType() != ODBCDB::DB2ODBC ) return 0;
    for( int i = 1; i <= pmfTable.columnCount(); ++i )
    {
        if( GString(pmfTable.column(i)->misc()).occurrencesOf("GENERATED ALWAYS") ) return 1;
    }
    return 0;
}

int ImportBox::resetGeneratedCols()
{
    if( resetNoneRB->isChecked() ) return 0;

    PmfTable pmfTable(m_pDSQL, currentTable);
    if( m_pDSQL->getDBType() != ODBCDB::DB2 && m_pDSQL->getDBType() != ODBCDB::DB2ODBC ) return 0;

    GString outMsg, err;

    m_pDSQL->initAll("select * from "+currentTable+" fetch first 1 rows only");
    if( m_pDSQL->numberOfRows() > 0 )
    {
        outMsg = "WARNING:\nTable is not empty, resetting GEN. ALWAYS columns may cause havoc.\n\nContinue?";
        if( QMessageBox::question(this, "PMF", outMsg, QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes )
        {
            return 1;
        }
    }

    for( int i = 1; i <= pmfTable.columnCount(); ++i )
    {
        if( GString(pmfTable.column(i)->misc()).occurrencesOf("GENERATED ALWAYS") )
        {
            GString start = pmfTable.column(i)->misc();
            if( start.occurrencesOf("(START WITH ") )
            {
                start = start.subString(start.indexOf("(START WITH ")+12, start.length()).strip();
                start = start.subString(1, start.indexOf(",")-1);
                GString cmd = "ALTER TABLE "+currentTable+" ALTER COLUMN " +pmfTable.column(i)->colName()+" RESTART WITH "+start;
                outMsg = "Column "+pmfTable.column(i)->colName()+" is GENERATED ALWAYS. Do you want to restart the counter, beginning with ["+start+"]?";

                if( resetAllRB->isChecked() )
                {
                    err = m_pDSQL->initAll(cmd);
                    if(err.length())msg(err);
                }
                else if( QMessageBox::question(this, "PMF", outMsg, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes )
                {
                    err = m_pDSQL->initAll(cmd);
                    if(err.length())msg(err);
                }
            }
        }
    }
    return 0;
}
