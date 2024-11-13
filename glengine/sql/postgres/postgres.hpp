//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt
//

#ifndef _POSTGRES_
#define _POSTGRES_

#ifdef  __IBMCPP__
#define MAKE_VA
#define NO_QT
#endif

#ifdef  _MSC_VER
#ifndef MAKE_VC
#define MAKE_VC
#endif
#endif


//#ifndef _IDSQL_
#include <idsql.hpp>
#include <growhdl.hpp>
//#endif



//************** Visual C++ ********************
#ifdef MAKE_VC
#define DllImport   __declspec( dllimport )
#define DllExport   __declspec( dllexport )
#define VCExport    DllExport
#include <windows.h>
#else
#define VCExport
#endif
//(************** Visual C++ END *****************

//************** Visual Age *********************
#ifdef MAKE_VA
#define NO_QT
#ifdef MakePostgres
#define dynsqlExp_Imp _Export
#else
#define dynsqlExp_Imp _Import
#pragma library( "GLEngine.LIB" )
#endif
#endif

//************** Visual Age END *****************

#include <libpq-fe.h>
#include "pmf_pgsql_def.hpp"

#define PSTGRS_BYTEA        BYTEA_PMF
#define PSTGRS_LOB          117
#define PSTGRS_XML          XML_PMF
#define PSTGRS_UUID         UUID_PMF

//For selects like "select 'X' from ..."
#define PSTGRS_ANON_TYPE    1313
#define PSTGRS_ANON_COL     "?column?"


// ******************************* CLASS **************************** //

#ifdef postgres
class postgresExp_Imp postgres
        #else
class postgres : public IDSQL
        #endif
{

    class RowData
    {
    private:
        GSeq <GString> rowDataSeq;
    public:
        RowData(){}
        ~RowData();
        void add(GString data);
        GString elementAtPosition(int pos);
        unsigned long numberOfElements();
    };

private:

    PGconn *m_pgConn;
    void deb(GString text);
    void deb(GString fnName, GString text);

    //QString uuidToString(const QVariant &v);
    int m_iMyInstance;

    ODBCDB m_odbcDB;
    GString m_strDB, m_strUID, m_strPWD, m_strHost, m_strPort, m_strEncoding, m_strOptions, m_strPwdCmd;
    signed int m_iNumberOfColumns;
    unsigned long m_iNumberOfRows;

    GString m_strCurrentDatabase;
    int m_iTruncationLimit;
    int m_iReadUncommitted;
    int m_iIsTransaction;


    GSeq <GRowHdl* > allRowsSeq;
    GRowHdl *m_pRowAtCrs;


    GSeq <GString> hostVarSeq;
    GSeq <short>   sqlTypeSeq;
    GSeq <short>   sqlVarLengthSeq;
    GSeq <short>   sqlIndVarSeq;
    GSeq <short>   sqlForBitSeq;
    GSeq <short>   sqlBitSeq;
    GSeq <short>   sqlLongTypeSeq;
    GSeq <short>   simpleColTypeSeq; //Simple type specifiers: LOB, XML, CHAR,...
    GSeq <long>    sqlLenSeq;

    GSeq <CON_SET*> m_seqConSet;
    GSeq<GString> headerSeq;
    GSeq<RowData*> rowSeq;
    GString m_strLastSqlSelectCommand;

    void clearSequences();


    void resetAllSeq();
    GString m_strLastError;
    int     m_iLastSqlCode;
    int writeToFile(GString fileName, GString data, int len);
    GString tabSchema(GString table, int removeDoubleQuotes = 0);
    GString tabName(GString table, int removeDoubleQuotes = 0);
    GString context(GString table);
    GString addQuotes(GString in);

    void createIndexRow(QTableWidget *pWdgt, int row,
                        GString id, GString name, GString cols, GString unq, GString crt, GString mod, GString dis);
    int getIdentityColParams(GString table, int *seed, int * incr);
    GString exportCsvBytea(GKeyVal *pKeyVal);
    GString getColumnsFromCreateStmt(GString stmt);
    void addToIdxSeq(GSeq <IDX_INFO*> *indexSeq, IDX_INFO *pIdx);
    //GString importCsvBytea(GString table, GString srcFile, GString logFile, GString delim, int hasHeader, int commitCount, int byteaAsFile);
    GString initAllInternal(GString message, long maxRows = -1, int getLen = 0, int readClobs = 0);
    GString getConstraint(GString sqlCmd);
    int tryFastCursor(GString sqlCmd);
    void convertToBin(GString inFile, GString outFile);
    GString sqlErrInternal();

    //void setSimpleColType(enum_field_types type);


    //New: Keep
    GString getForeignKeyStatement(GString table, GString keyName );
    int loadFileIntoBuf(GString fileName, char** fileBuf, int *size);
    int loadFileIntoBufNoConvert(GString fileName, char** fileBuf, int *size);
    void impExpLob();
    long importBlob(GSeq <GString> *fileSeq);
    GString exportBlob(unsigned int oid, GString target);
    //int loadFileIntoBuf(GString fileName, unsigned char** fileBuf, unsigned long *size)
    GString fillHostVarSeq(GString message);
    GString createConnectionString(CON_SET * pCs);

    GString connectInternal(CON_SET * pCs);
    void clearPgRes();

    int m_iReadClobData;

    GDebug * m_pGDB;

    PGresult * m_pRes;
public:                              // Public section
    //VCExport postgres(postgres & aODBC);
    //VCExport postgres &operator=(postgres aODBC);


    VCExport postgres();
    VCExport ~postgres();
    VCExport postgres(postgres const & o);
    VCExport postgres* clone() const;



    VCExport ODBCDB getDBType()
    {
        return POSTGRES;
    }
    VCExport int getConnData(GString * db, GString *uid, GString *pwd) const;
    VCExport GString getDBTypeName();
    VCExport void setDBType(ODBCDB dbType){ m_odbcDB = dbType;}
    VCExport GString connect(GString db, GString uid, GString pwd, GString host = "localhost", GString port = "5432");
    VCExport GString connect(CON_SET * pCs);
    VCExport int disconnect();
    VCExport GString initAll(GString message, long maxRows = -1, int getLen = 0);
    GString initAllCrs(GString message, long maxRows,  int getLen);
    VCExport int commit();
    VCExport int rollback();
    VCExport unsigned int  numberOfColumns();
    VCExport unsigned long numberOfRows();
    VCExport GString rowElement(unsigned long row, int col);
    VCExport GString rowElement(unsigned long row, GString hostVar);
    VCExport GString hostVariable(int col);


    VCExport GString currentCursor(GString filter, GString command, long curPos, short commitIt = 1,
                                   GSeq <GString> *fileList = NULL, GSeq <long> *lobType = NULL);


    VCExport int sqlCode();
    VCExport GString sqlError();



    VCExport int getTabSchema();
    VCExport int getTables(GString schema);
    VCExport void convToSQL(GString & input);
    VCExport short sqlType(const short & col);
    VCExport short sqlType(const GString & colName);
    VCExport GString realName(const short & sqlType);

    VCExport int initRowCrs();
    VCExport int nextRowCrs();
    VCExport GString dataAtCrs(int col);


    VCExport int getAllTables(GSeq <GString > * tabSeq, GString filter = "");

    VCExport GString descriptorToFile( GString cmd, GString &blobFile, int * outSize );

    VCExport signed long getCost();
    VCExport void setStopThread(int stop){PMF_UNUSED(stop);}

    VCExport int execByDescriptor( GString cmd, GSeq <GString> *dataSeq);
    VCExport int execByDescriptor( GString cmd, GSeq <GString> *dataSeq, GSeq <int> *typeSeq,
                                   GSeq <short>* sqlVarLengthSeq, GSeq <int> *forBitSeq );


    VCExport int isNumType(int pos);
    VCExport int isDateTime(int pos);
    VCExport int hasForBitData();
    VCExport int isForBitCol(int pos);
    VCExport int isBitCol(int i);
    VCExport int isXMLCol(int pos);
    VCExport int isLOBCol(int i);
    VCExport int isNullable(int i);
    VCExport int isLongTypeCol(int i);
    VCExport int isFixedChar(int i);
    VCExport int simpleColType(int i);
    VCExport void setCharForBit(int val){PMF_UNUSED(val);}
    VCExport void setReadUncommitted(short readUncommitted = 1);

    VCExport short bindIt(GString, GString, GString, GString, GString){ return 0;}


    VCExport long uploadBlob(GString cmd, GSeq <GString> *fileList, GSeq <long> *lobType);
    VCExport long uploadBlob(GString cmd, char * buffer, long size);
    VCExport int getDataBases(GSeq <CON_SET*> *dbList);



    long dataLen(const short &pos);

    VCExport int positionOfHostVar(const GString& hostVar);
    //Pack this into helper libs...
    VCExport int  deleteTable(GString tableName);

    VCExport void setCurrentDatabase(GString db);
    VCExport GString currentDatabase();

    long retrieveBlob( GString cmd, GString &blobFile, int writeFile = 1 );


#ifdef  QT4_DSQL
    VCExport GString getIdenticals(GString table, QWidget* parent, QListWidget *pLB, short autoDel);
    VCExport GString  fillIndexView(GString table, QWidget* parent, QTableWidget *pWdgt);
    VCExport void writeToLB(QListWidget * pLB, GString message);
#endif


    VCExport int forceApp(int appID){ PMF_UNUSED(appID);return -1;}
    VCExport void setCLOBReader(short readCLOBData );

    VCExport int getSysTables(){ return -1;}
    VCExport  int getColSpecs(GString table, GSeq<COL_SPEC*> *specSeq);
    VCExport  int getTriggers(GString table, GString *text);
    VCExport GSeq <IDX_INFO*> getIndexeInfo(GString table);
    void getIndexInfoExtended(GString cmd, GSeq <IDX_INFO*> *indexSeq, GString tableName);
    void getIndexInfo(GString cmd, GSeq <IDX_INFO*> *indexSeq, GString tableName = "");
    VCExport GSeq <GString> getTriggerSeq(GString table);

    VCExport GString getChecks(GString, GString = "");
    VCExport GSeq <GString> getChecksSeq(GString, GString);

    VCExport  int getUniqueCols(GString table, GSeq <GString> * colSeq);
    VCExport void createXMLCastString(GString &xmlData);
    VCExport int isBinary(unsigned long row, int col);
    VCExport int  isNull(unsigned long row, int col);
    VCExport int isTruncated(unsigned long row, int col);
    VCExport void setTruncationLimit(int limit);
    VCExport int uploadLongBuffer( GString cmd, GString data, int isBinary );
    VCExport GString cleanString(GString in);
    VCExport void getResultAsHEX(int asHex);
    VCExport void setGDebug(GDebug *pGDB);
    VCExport int exportAsTxt(int mode, GString sqlCmd, GString table, GString outFile, GSeq <GString>* startText, GSeq <GString>* endText, GString *err);
    VCExport GString fillIndexView(GString table, int showRaw = 0);
    VCExport GString fillChecksView(GString table, int showRaw = 0);
    VCExport  int getHeaderData(int pos, GString * data);
    VCExport  int getRowData(int row, int col, GString * data);
    VCExport unsigned long getRowDataCount();
    VCExport unsigned long getHeaderDataCount();
    VCExport int isTransaction();
    VCExport void setDatabaseContext(GString context);
    VCExport int hasUniqueConstraint(GString tableName);
    VCExport GString getDdlForView(GString tableName);
    VCExport void setAutoCommmit(int commit);
    VCExport void currentConnectionValues(CON_SET * conSet);
    VCExport GString lastSqlSelectCommand();
    VCExport TABLE_PROPS getTableProps(GString tableName);
    VCExport int tableIsEmpty(GString tableName);
    VCExport int deleteViaFetch(GString tableName, GSeq<GString> * colSeq, int rowCount, GString whereClause);
    VCExport GString allPurposeFunction(GKeyVal * pKeyVal);
    VCExport GString setEncoding(GString encoding);
    VCExport void getAvailableEncodings(GSeq<GString> *encSeq);
    VCExport GString reconnect(CON_SET *pCS);
};

#endif







