//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt 2000
//

#include <qapplication.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <QListWidget>
#include <gstring.hpp>
#include <gthread.hpp>
#include "threadBox.h"
#include "pmfSchemaCB.h"
#include <qfiledialog.h>
#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qcheckbox.h>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QTextBrowser>

#include <dsqlplugin.hpp>

#include "expImpOptions.h"
#include "pmfDropZone.h"
#include "pmfTable.h"

#ifndef _IMPORTBOX_
#define _IMPORTBOX_

static GString ERR_COL_NUMBER = "Number of columns does not match data";

enum IMPORT_TYPE
{
    IMP_FILE_IXF,
    IMP_FILE_DEL,
    IMP_FILE_WSF,
    IMP_FILE_UNKNOWN
};

class ImportBox : public QDialog
{
    class ImportThread : public GThread
    {
    public:
        virtual void run();
        void setOwner( ImportBox * aImport ) { myImport = aImport; }
    private:
        ImportBox * myImport;
    };


    Q_OBJECT
public:

    ImportBox(DSQLPlugin* pDSQL, QWidget *parent, GString currentSchema, GString currentTable, GString *prevDir, int hideSysTabs = 0);
    ~ImportBox();
    short fillLB(GSeq <GString> * tableNameSeq);
    void setSchema(GString schema);
    void callImport(GString fullPath);
    void startImport();
    void startDB2Import();
    void startDB2CsvImport();
    void startPGSQLImport();
    void setImportTypeFromFileType(GString file);
    GString setFilesToImport(GSeq<GString> fileSeq);
    void setGDebug(GDebug * pGDeb){ m_pGDeb = pGDeb; }

protected:
    void closeEvent(QCloseEvent* );
    void keyPressEvent(QKeyEvent *event);

private slots:
    void OKClicked();
    void CancelClicked();
    void getFileClicked();
    void deleteErrLog();
    void schemaSelected(int index);
    void timerEvent();
    void optionSelected();
    void typeSelected();
    void optionsClicked();
    void filesDropped();

private:

    QFont f;
    QLineEdit *tableNameLE;
    PmfDropZone *fileNameDropZone;
    int m_sqlErc;
    GString m_sqlErrTxt;
    GString m_strLogFile;
    void displayLog();
    GString newCodePage(GString path);
    bool isHADRorLogArchMeth();
    void runLoadChecks();
    void createExpImpOptions();
    GString getTargetTable();    
    GString tableFromFile(GString file);
    GString schemaFromFile(GString file);
    GString typeFromFile(GString file);
    int canMapToTable(GString table);
    IMPORT_TYPE importTypeFromFile(GString file);
    int checkForSameImportType(GSeq<GString> *fileSeq);
    int showMultiImport(GSeq<GString> *fileSeq);
    void handleImportFiles();
    void setComboBoxesFromFile(GString input);
    void setImportOptionsEnabled(bool enabled);
    int resetGeneratedCols();
    int m_iHideSysTables;
    int hasGeneratedColumns();
    GString createPgsqlCsvInsertCmd(PmfTable * pmfTable, GString line, GString delim, int byteaAsFile, int xmlAsFile);
    GString createDb2CsvInsertCmd(PmfTable * pmfTable, GString line, GString delim, int byteaAsFile, int xmlAsFile);
    void writeLog(GString msg);
    int loadFileIntoBuf(GString fileName, char* fileBuf, int *size);
    int loadFileIntoBuf(GString fileName, GString* data);
    int runCsvChecks(PmfTable * pmfTable, GString line, GString delim);

    GString iSchema;
    QPushButton * ok, * cancel, *getFileB, *delLog;
    QButtonGroup * formGroup, * typeGroup, *actionGroup;
    QRadioButton * ixfRB, *delRB, *wsfRB, *crtRB, *insRB, *updRB, *replRB, *repCrRB, *importRB, *loadRB, *csvRB;
    QRadioButton *resetAllRB, *resetNoneRB, *resetAskRB;
    QPushButton * optionsB;
    QListWidget* tableLB;
    //QTextBrowser * errorLB;
    QTextEdit * errorLB;
    GString currentSchema, currentTable;
    PmfSchemaCB * schemaCB;
    void fillLB(GString schema, GString table = "");
    void msg(QString message){QMessageBox::information(this, "Import", message);  return;}
    GString *m_gstrPrevDir;
    ImportThread * aThread;
    ThreadBox * tb;
    QTimer *timer;
    void setWhatsThisText();
    DSQLPlugin* m_pDSQL;
    GDebug * m_pGDeb;
    void setButtons();
    ExpImpOptions * m_pExpImpOptions;
    GString m_importMsg;
    GSeq<GString> m_impFileSeq;

};

#endif
