
#ifndef pmf_H
#define pmf_H

#include <qglobal.h>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QApplication>
#else
#include <QtGui/QApplication>
#endif
//#include <QtGui/QWidget>

#include <QComboBox>
#include <QTableWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QToolTip>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QSettings>
#include <QGroupBox>

#include <dsqlplugin.hpp>

#include <gstring.hpp>
#include <gdebug.hpp>
#include "tabEdit.h"
#include <idsql.hpp>
#include "db2menu.h"
#include "gseq.hpp"
#include "downloader.h"




class Pmf : public QMainWindow
{
    Q_OBJECT
    class TheThread : public GThread
    {
       public:
          virtual void run();
          void setOwner( Pmf * aPmf ) { myPmf = aPmf; }
       private:
          Pmf * myPmf;
    };

    class MyThread : public GThread //"public" inheritance: Due to a bug in gcc 2.96 - 3.0.1
    {
        public:
            virtual void run();
            void setOwner(Pmf *pPmf){ myPmf = pPmf; }
        private:
            Pmf * myPmf;
    };

public:
    Pmf(GDebug *pGDeb = NULL, int threaded = 0 );
    ~Pmf();
	void getGeometry(int * x, int * y, int * w, int * h );
	int charForBit(){return m_iCharForBit;}
    void setLastSelectedSchema(GString context, GString schema);
    DSQLPlugin* getConnection();
    TabEdit * currentTabEdit();
    int checkTableSet();
    void checkMigration();
    void closePMF();
    void addToHostVarSeq(GSeq <GString> * pSeq);
    QStringList completerStringList();
    void getNewVersion();
    void setColorScheme(PmfColorScheme scheme, QPalette palette);
    PmfColorScheme getColorScheme();
    void refreshTabOrder();
    void setFontFromSettings();

    GSeq <GString>* hostVarSeq();
    GSeq <GString>* sqlCmdSeq();
    GString m_gstrCurrentVersion;

#if defined(MAKE_VC) || defined (__MINGW32__)
    HWND getMainWindowHandle();
#endif

private:
	void createGUI();
	void msg(GString txt);
    int closeDBConn();
	QMenu* createStyleMenu();
	QMenu* createCharForBitMenu();
    //QMenu * createEncodingMenu();
    QMenu* createRestoreMenu();
	GString currentSchema();
	GString currentTable();
    int restorePrevious();
    void savePrevious();
    GString restoreFileName(int checkForOldFiles = 0);
    GString histTableName();
    void createMenu(GString dbTypeName);
    void createCheckBoxActions();
    void setAndConnectActions(QSettings *settings,  QAction* action, QString name, QString defVal= "");
    GString getVersionFromHTML(GString data);
    void createInfoArea();
    int closeTab(int index);
    TabEdit * getCurrentTabWdgt();
    QStringList availableEncodings();
    GString saveEncoding();
    GString getEncoding();
    GString getPwdFromCmd(GString cmd, GString *pwd);
	
	QMenu * m_stylesMenu;
	QMenu * m_charForBitMenu;
    QMenu * m_encodingMenu;
    QMenu * m_restoreMenu;
    GString m_gstrDBType;
	GString m_gstrDBName;
	GString m_gstrUID;
	GString m_gstrPWD;
    GString m_gstrPort;
	GString m_gstrNODE;
    GString m_gstrConnectionColor;
	int m_iCurrentTab;

	int m_iTabIDCounter;
	QTabWidget * m_tabWdgt;

	void getBookmarks();
    QMenu * m_mnuBookmarkMenu;
    QMenu *m_mnuMainMenu;
    QMenu * m_mnuAdmMenu;
    QMenu * m_mnuTableMenu;
    QMenu * m_mnuSettingsMenu;
    QMenu * m_mnuMelpMenu;
	enum { MaxBookmarkActs = 50 };
	QAction *bmActs[MaxBookmarkActs];
    QAction *m_actTextCompleter;
	QAction *m_actCountAllRows;
    QAction *m_actConvertGuid;
    QAction *m_actAutoLoadTable;
    QAction *m_actReadUncommitted;
    QAction *m_actHideSysTabs;
    QAction *m_actUseEscKey;
    QAction *m_actVersionCheck;
    QAction *m_actEnterToSave;
    QAction *m_actRefreshOnFocus;
    QAction *m_actShowCloseOnTabs;
    QLineEdit * downloadInfoLE;
	GString m_gstrPrevImportPath;	
	GString m_gstrPrevExportPath;	
	void deb(GString msg);
	void closeEvent(QCloseEvent * event);
    void keyPressEvent(QKeyEvent *event);


    int m_iThreaded;
	int m_iCharForBit;
    int m_iRestore;
    int m_iShowing;
    int m_iReconnectWaitTime;
    GString m_gstrPwdCmd;
    GString m_strHistTableName;
    GString m_strLastSelectedSchema;
	GString m_strLastSelectedContext;

    DSQLPlugin * m_pIDSQL;
    Db2Menu *m_pDB2Menu;
	GDebug * m_pGDeb;
    int m_iForceClose;
    GSeq <GString> m_hostVarSeq;
    GSeq <GString> m_sqlCmdSeq;
    MyThread * aThread;
    QTimer *versionCheckTimer;
    QTimer *reconnectTimer;

    Downloader * m_pDownloader;
    QPushButton * downloadCancelButton;
    QGroupBox * downloadInfoBox;

    QPushButton * reconnectNowBt;
    QGroupBox * reconnectInfoBox;
    QLineEdit * reconnectInfoLE;


    PmfColorScheme m_iColorScheme;
    QPalette m_qPalette;
    GSeq <CHECKBOX_ACTION *> m_cbMenuActionSeq;
    int timerCount;
    //QAction * m_menuBarBookmarkAction;


public slots:
    void createNewTab(GString cmd = "", int asNext = 1);
    void closeTabClicked(int index);
//    void onApplicationStateChanged(Qt::ApplicationState state);
    
private slots:
	void nextTab();
	void prevTab();	
    int closeCurrentTab();
    void exportData();
	void importData();	
	void catalogDBs();
	void deleteTable();
    void createDDLs();
	void findIdenticals();
	void runstats();
	void getTabSpace();
	void tableSizes();
	void setPmfFont();
    void setConnectionColor();
    void resetPmfFont();
	void snapShot();
	void showInfo(); 
	void setConfig();
	void createDDL();
	void quitPMF();
	void queryDB();
	void addBookmark();
	void editBookmark();	
	void setBookmarkData();
    void checkBoxAction();
	void setConnections();
    void setEncoding();
	void callLogin();
	void setStyle();
	void setCharForBit();
    void setRestore();
    void showHelp();
	void showDebug();
    GString checkForUpdate(int todayOnly = 0);
    void versionCheckTimerEvent();
    void reconnectTimerEvent();
    void checkDownloadSize();
    void handleDownloadResult();
    void downloadCancelled();    
    void addNewTab();       
    void bookmarkMenuClicked();
    void showDatabaseInfo();
    void deleteTableContents();

public slots:	
    int loginClicked();
	void curTabChg(int index);
    void reconnectNowClicked();

  protected:
    void showEvent( QShowEvent * evt);
    //bool eventFilter(QObject*, QEvent *event);

};

#endif 
