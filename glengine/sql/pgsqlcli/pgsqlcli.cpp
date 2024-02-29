//
//  This file is part of PMF (Poor Man's Flight), see README.PMF for details
//  Author: Gregor Leipelt (gregor@leipelt.de)  (C) G. Leipelt
//

#ifndef _PGSQLCLI_
#include <pgsqlcli.hpp>
#endif

#include <QVariant>
#include <QStringList>
#include <QDebug>
#include <QUuid>
#include <QMessageBox>

#include <idsql.hpp>
#include <gdebug.hpp>
#include <gfile.hpp>
#include <gstuff.hpp>




#ifdef MAKE_VC
/////#include <windows.h>
#endif

#ifndef max
#define max(A,B) ((A) >(B) ? (A):(B))
#endif

///////////////////////////////////////////
//Global static instance counter.
//Each instance saves its private instance value in m_iMyInstance.
static int m_pgsqlCLIobjCounter = 0;

#define XML_MAX 250



/***********************************************************************
* This class can either be instatiated or loaded via dlopen/loadlibrary
***********************************************************************/

//Define functions with C symbols (create/destroy instance).
#ifndef MAKE_VC
extern "C" pgsqlCLI* create()
{
    //printf("Calling pgsqlCLI creat()\n");
    return new pgsqlCLI();
}
extern "C" void destroy(pgsqlCLI* pODBCSQL)
{
    if( pODBCSQL ) delete pODBCSQL ;
}
#else
extern "C" __declspec( dllexport ) pgsqlCLI* create()
{
    //printf("Calling pgsqlCLI creat()\n");
    _flushall();
    return new pgsqlCLI();
}
extern "C" __declspec( dllexport ) void destroy(pgsqlCLI* pODBCSQL)
{
    if( pODBCSQL ) delete pODBCSQL ;
}
#endif
/***********************************************************
* CLASS
**********************************************************/

pgsqlCLI::pgsqlCLI(pgsqlCLI  const &o)
{
    m_iLastSqlCode = 0;
    m_pgsqlCLIobjCounter++;
    m_iTruncationLimit = 500;
    m_iMyInstance = m_pgsqlCLIobjCounter;
    m_pGDB = o.m_pGDB;
    deb("CopyCtor start");

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_SQLEnv);
    deb("CopyCtor, ret from SQLAllocEnv: "+GString(ret));
    SQLSetEnvAttr(m_SQLEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    ret = SQLAllocHandle(SQL_HANDLE_DBC, m_SQLEnv, &m_SQLHDBC);
    deb("CopyCtor, ret from SQLAllocDBC: "+GString(ret));

    //m_SQLEnv = o.m_SQLEnv;
    //m_SQLHDBC = o.m_SQLHDBC;
    m_SQLHSTMT = SQL_NULL_HSTMT;
    m_odbcDB  = PGSQLCLI;
    m_iReadUncommitted = o.m_iReadUncommitted;
    m_strContext = o.m_strContext;
    m_iIsTransaction = o.m_iIsTransaction;
    m_strDB = o.m_strDB;
    m_strHost = o.m_strHost;
    m_strPWD = o.m_strPWD;
    m_strUID = o.m_strUID;
    m_strPort  = o.m_strPort;
    m_strCltEnc = o.m_strCltEnc;
    m_strLastSqlSelectCommand = o.m_strLastSqlSelectCommand;
    m_strCurrentDatabase = o.m_strCurrentDatabase;

    //ret = SQLAllocHandle(SQL_HANDLE_DBC, m_SQLEnv, &m_SQLHDBC);
    deb("Copy CTor done");


    GString db, uid, pwd;
    o.getConnData(&db, &uid, &pwd);
    this->connect(m_strDB, m_strUID, m_strPWD, m_strHost, m_strPort);
    deb("Context to use: "+m_strContext);
    if( m_strContext.length() ) setDatabaseContext(m_strContext);
    if( m_iReadUncommitted ) readRowData("set transaction isolation level read uncommitted");
    else readRowData("set transaction isolation level REPEATABLE READ");
}

pgsqlCLI::pgsqlCLI()
{
    m_pgsqlCLIobjCounter++;
    //m_IDSQCounter++;
    m_iTruncationLimit = 500;
    m_iMyInstance = m_pgsqlCLIobjCounter;
    m_SQLEnv = SQL_NULL_HENV;
    m_SQLHDBC = SQL_NULL_HDBC;
    m_SQLHSTMT = SQL_NULL_HSTMT;
    m_iLastSqlCode = 0;
    m_iIsTransaction = 0;
    m_strPort = 5432;
    m_strEncoding = "";


    m_odbcDB  = PGSQLCLI;

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_SQLEnv);
    if(!ret) ret = SQLSetEnvAttr(m_SQLEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    //if(!ret) ret = SQLAllocHandle(SQL_HANDLE_DBC, m_SQLEnv, &m_SQLHDBC);

    m_strDB ="";
    m_strUID ="";
    m_strPWD = "";
    m_strHost = "";
    m_pGDB = NULL;
    m_strLastSqlSelectCommand = "";
    m_iReadUncommitted = 0;
    m_strCurrentDatabase = "";
}
pgsqlCLI* pgsqlCLI::clone() const
{
    return new pgsqlCLI(*this);
}

pgsqlCLI::~pgsqlCLI()
{
    deb("Dtor start, current count: "+GString(m_pgsqlCLIobjCounter));
    disconnect();
    SQLFreeHandle(SQL_HANDLE_STMT, m_SQLHSTMT);
    SQLDisconnect(m_SQLHSTMT);
    SQLFreeHandle(SQL_HANDLE_DBC, m_SQLHDBC);
    SQLFreeHandle(SQL_HANDLE_ENV, m_SQLEnv);
    m_pgsqlCLIobjCounter--;
    //m_IDSQCounter--;
    deb("Dtor, clearing RowData...");
    clearSequences();
    deb("Dtor done, current count: "+GString(m_pgsqlCLIobjCounter));
}

int pgsqlCLI::getConnData(GString * db, GString *uid, GString *pwd) const
{
    *db = m_strDB;
    *uid = m_strUID;
    *pwd = m_strPWD;
    return 0;
}


GString pgsqlCLI::connect(GString db, GString uid, GString pwd, GString host, GString port)
{
    SQLRETURN ret;
    deb("connect to DB: "+db+", uid: "+uid+", host: "+host+", port: "+port);
    m_strDB = db;
    m_strUID = uid;
    m_strPWD = pwd;
    m_strNode = host;
    m_strHost = host;
    m_strPort = port;
    m_strCltEnc = "";
    m_strCurrentDatabase = db;
    deb("connection data: "+m_strDB+", uid: "+m_strUID+", host: "+host+", port: "+port);
    /*
SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_SQLEnv);

if (!SQL_SUCCEEDED(ret)) return sqlError();

SQLSetEnvAttr(m_SQLEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

ret = SQLAllocHandle(SQL_HANDLE_DBC, m_SQLEnv, &m_SQLHDBC);
if (!SQL_SUCCEEDED(ret)) return sqlError();
*/
    ret = SQLAllocHandle(SQL_HANDLE_DBC, m_SQLEnv, &m_SQLHDBC);
    if( !ret) ret = SQLConnect(m_SQLHDBC, (SQLCHAR*)db, SQL_NTS, (SQLCHAR*)uid, SQL_NTS,(SQLCHAR*)pwd, SQL_NTS);

    deb("connection err: "+GString(ret));
    if (!SQL_SUCCEEDED(ret)) errorState(m_SQLHDBC);
    if (!SQL_SUCCEEDED(ret)) return sqlError();
    SQLSetConnectAttr(m_SQLHDBC, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)true, 0);

    return "";
}

GString pgsqlCLI::connect(CON_SET * pCs)
{
    return this->connect(pCs->DB, pCs->UID, pCs->PWD, pCs->Host, pCs->Port);
}


int pgsqlCLI::disconnect()
{
    deb("Disconnecting and closing.");
    SQLDisconnect(m_SQLHSTMT);
    return 0;
}
GString pgsqlCLI::initAll(GString message, unsigned long maxRows,  int getLen)
{
    return readRowData(message, maxRows,  getLen, 0);
}

GString pgsqlCLI::readRowData(GString message, unsigned long maxRows,  int getLen, int getFullXML)
{
    deb("::initAll, cmd: "+message);
    m_iLastSqlCode = 0;

    if( m_SQLHDBC == SQL_NULL_HDBC )
    {
        deb("::initAll: No handle, quitting.");
        return "";
    }


    m_ulMaxRows = maxRows;
    m_ulFetchedRows = 0;

    //Clear all internal sequences:
    resetAllSeq();

    SQLRETURN ret = 0;
    SQLSMALLINT columns;

    //SQLFreeHandle(SQL_HANDLE_DBC, m_SQLHDBC);
    SQLFreeHandle(SQL_HANDLE_STMT, m_SQLHSTMT);
    ret = SQLAllocHandle(SQL_HANDLE_STMT, m_SQLHDBC, &m_SQLHSTMT);
    deb("::initAll, SQLAllocHandle, rc: "+GString(ret));
    if(ret)
    {
        m_iLastSqlCode = ret;
        return sqlError();
    }

    ret = SQLExecDirect(m_SQLHSTMT, (SQLCHAR*)message, SQL_NTS);

    deb("::initAll, SQLExecDirect, rc: "+GString(ret));
    if(ret)
    {
        m_iLastSqlCode = ret;
        freeStmt(m_SQLHSTMT);
        if( m_iLastSqlCode == 100) return "SQLCode 100: No row(s) found.";
        return sqlError();
    }


    ret = SQLNumResultCols(m_SQLHSTMT, &columns);
    deb("::initAll, SQLNumResultCols, rc: "+GString(ret)+", cols: "+GString(columns)+", m_iLastSqlCode: "+GString(m_iLastSqlCode));
    if(ret)
    {
        freeStmt(m_SQLHSTMT);
        m_iLastSqlCode = ret;
        return sqlError();
    }
    m_iNumberOfColumns = columns;


    SQLLEN rowCount;
    SQLRowCount(m_SQLHSTMT, &rowCount);

    deb("::initAll, checkInsUpdDel,  ret: "+GString(ret)+", m_iNumberOfColumns: "+GString(m_iNumberOfColumns)+", m_iLastSqlCode: "+GString(m_iLastSqlCode));
    //Apparently INS,UPD,DEL
    if( m_iNumberOfColumns == 0 )
    {
        m_iLastSqlCode = ret;
        deb("::initAll, is INS/UPD/DEL, m_iLastSqlCode: "+GString(m_iLastSqlCode)+", rowCount: "+GString((int)rowCount));
        if( m_iLastSqlCode ) return sqlError();
        m_iIsTransaction = 1;
        if( rowCount == 0 )
        {
            m_iLastSqlCode = 100;
            return "No rows were affected";
        }
        return "";
    }

    GRowHdl * pRow;
    getColInfo(&m_SQLHSTMT);
    if(getLen)
    {
        for(int i = 1; i <= m_iNumberOfColumns; ++i ) sqlLenSeq.add(hostVariable(i).length()+1);
    }

    char *buf;
    GString data;
    long maxLen;

    while (SQL_SUCCEEDED(ret = SQLFetch(m_SQLHSTMT)))
    {
        SQLLEN len = 0;
        if( maxRows > 0 && m_iNumberOfRows >= maxRows ) break;
        pRow = new GRowHdl;
        int bufLen;
        short isNull = 0;

        for ( int i = 1; i <= m_iNumberOfColumns; i++ )
        {
            deb("::readRowData, col: "+GString(i));
            if( handleLobsAndXmls(i, &data, getFullXML, &isNull) )
            {
                deb("::readRowData, col "+GString(i)+" is LOB/XML");
                pRow->addElement(data, isNull);
            }
            else
            {
                deb("::readRowData, col: "+GString(i)+" no LOB/XML data. sqlVarLengthSeq has "+GString(sqlVarLengthSeq.numberOfElements())+" elements");
                bufLen = sqlVarLengthSeq.elementAtPosition(i)+1;

                buf = new char[bufLen];
                ret = SQLGetData(m_SQLHSTMT, i, SQL_C_CHAR, buf, bufLen, &len);

                //deb("::initAll, row: "+GString(m_iNumberOfRows)+", col: "+GString(i)+", ret: "+GString(ret)+", len: "+GString(len)+", bufLen: "+GString(bufLen)+", blob: "+GString(lobSeq.elementAtPosition(i))+", xml: "+GString(xmlSeq.elementAtPosition(i)));
                deb("::initAll, row: "+GString(m_iNumberOfRows)+", col: "+GString(i)+", ret: "+GString(ret)+", len: "+GString((int)len)+", bufLen: "+GString(bufLen));
                if (len == SQL_NULL_DATA)  pRow->addElement("NULL");
                else
                {
                    deb("buf: "+GString(buf));
                    data = buf;
                    if( sqlTypeSeq.elementAtPosition(i) == SQLSRV_DATETIME ) handleDateTimeString(data);
                    if( !isNumType(i))pRow->addElement("'"+data+"'");
                    else pRow->addElement(data);
                }
                delete[] buf;
            }
            if( getLen )
            {
                maxLen = max((signed) data.strip("'").strip().length()+1, sqlLenSeq.elementAtPosition(i));
                deb("initAll: Col "+GString(i)+", setting max: "+GString(maxLen));
                sqlLenSeq.replaceAt(i, maxLen);
            }
        }
        allRowsSeq.add( pRow );
        m_iNumberOfRows++;
    }
    SQLCloseCursor(m_SQLHSTMT);
    m_strLastSqlSelectCommand = message;
    deb("::initAll, err fetch: "+GString(ret));
    freeStmt(m_SQLHSTMT);
    if( ret && ret != 100) return m_strLastError;
    return "";
}
//Workaround for WIN:
void pgsqlCLI::handleDateTimeString(GString &in)
{
    if( in.length() == 23 && in.occurrencesOf(' ') == 1 ) in.replaceAt(11, 'T');
}


/*****************************************************
* 1. getFullXML is currently unused and anyway will only be useful w/ TDS 7.1/8.0
*
* 2. TDS7.2: len will be returned as -4
*
* So at the moment (using TDS 7.2) we return the buf although len is -4
*
*****************************************************/

int pgsqlCLI::handleLobsAndXmls(int col, GString * out, int getFullXML, short* isNull)
{
    PMF_UNUSED(getFullXML);

    deb("::handleLobsAndXmls, col: "+GString(col)+", lob: "+GString(lobSeq.elementAtPosition(col))+", xml: "+GString(xmlSeq.elementAtPosition(col)));

    *out = "";
    *isNull = 0;

    if( isLOBCol(col) == 0 && isXMLCol(col) == 0 ) return 0;



    SQLLEN len;
    char* buf = new char[XML_MAX+1];


#ifdef USE_TDS71
    deb("::handleLobsAndXmls: TDS71");
    SQLGetData( m_SQLHSTMT, col, SQL_C_CHAR, buf, 0, &len);
#else
    deb("::handleLobsAndXmls: TDS72");
    //SQLGetData( m_SQLHSTMT, col, SQL_C_CHAR, 0, 0, &len);
#endif


    if (len == SQL_NULL_DATA)
    {
        *out = "NULL";
        *isNull = 1;
    }
    else if( lobSeq.elementAtPosition(col) == 1 ) *out = "@DSQL@BLOB";
    else if( lobSeq.elementAtPosition(col) == 2 ) *out = "@DSQL@CLOB";
    else if( xmlSeq.elementAtPosition(col) )
    {
#ifdef USE_TDS71
        int ret = 0;
        GString xml;
        while( 1 )
        {
            memset(buf, 0, sizeof(buf));
            ret = SQLGetData( m_SQLHSTMT, col, SQL_C_CHAR, buf, XML_MAX, &len);
            xml += GString(buf);
            if( ret == 0  || !getFullXML ) break;
        }
        *out = "'"+xml+"'";
#else
        SQLGetData( m_SQLHSTMT, col, SQL_C_CHAR, buf, XML_MAX, &len);
        *out = "'"+GString(buf)+"'";
#endif
    }
    delete [] buf;
    return 1;
}
/*
GString pgsqlCLI::initAll(GString message, unsigned long maxRows,  int getLen)
{
deb("::initAll, cmd: "+message);


if( m_SQLHDBC == SQL_NULL_HDBC )
{
    deb("::initAll: No handle, quitting.");
    return "";
}

m_ulMaxRows = maxRows;
m_ulFetchedRows = 0;

//Clear all internal sequences:
resetAllSeq();

SQLRETURN ret;
SQLSMALLINT columns;
SQLHSTMT     stmt;

ret = SQLAllocHandle(SQL_HANDLE_STMT, m_SQLHDBC, &stmt);
deb("::initAll, SQLAllocHandle, rc: "+GString(ret));
if(ret) return sqlError();

ret = SQLExecDirect(stmt, (SQLCHAR*)message, SQL_NTS);
deb("::initAll, SQLExecDirect, rc: "+GString(ret));
if(ret) return sqlError();


ret = SQLNumResultCols(stmt, &columns);
deb("::initAll, SQLNumResultCols, rc: "+GString(ret)+", cols: "+GString(columns));
if(ret) return sqlError();
m_iNumberOfColumns = columns;

GLineHdl * pLine;
getColInfo(&stmt);

char *buf;

while (SQL_SUCCEEDED(ret = SQLFetch(stmt)))
{
    SQLLEN len = 0;
    if( maxRows > 0 && m_iNumberOfRows >= maxRows ) break;
    pLine = new GLineHdl;
    int bufLen;

    for ( int i = 1; i <= m_iNumberOfColumns; i++ )
    {
        bufLen = sqlVarLengthSeq.elementAtPosition(i)+1;
        buf = new char[bufLen];
        ret = SQLGetData(stmt, i, SQL_C_CHAR, buf, bufLen, &len);
        if (!ret)
        {
            if (len == SQL_NULL_DATA)  pLine->addElement("NULL");
            else if( !isNumType(i))pLine->addElement("'"+GString(buf, len)+"'");
            else pLine->addElement(buf);
        }
        delete[] buf;
    }
    allRowsSeq.add( pLine );
    m_iNumberOfRows++;
}
SQLFreeStmt(stmt, SQL_CLOSE);
return sqlError();
}
*/


int pgsqlCLI::getColInfo(SQLHSTMT * stmt)
{
    SQLCHAR         colName[128];
    SQLSMALLINT     sqlType;
    SQLSMALLINT     colSize;
    SQLSMALLINT     nullable;
    SQLULEN         size;
    SQLSMALLINT     scale;

    deb("::getColInfo start");
    //SQLINTEGER      outlen[MAXCOLS];
    //SQLCHAR *       data[MAXCOLS];
    //SQLINTEGER      displaysize;

    for (int i = 1; i <= m_iNumberOfColumns; i++)
    {

        SQLDescribeCol (*stmt, i, colName, sizeof (colName),  &colSize, &sqlType, &size, &scale, &nullable);
        deb("Describe for "+GString(colName)+": colSize "+GString(colSize)+", type: "+GString(sqlType)+", size: "+GString((int)size));
        hostVarSeq.add(GString(colName, colSize).strip().strip("'"));
        sqlTypeSeq.add(sqlType);

        sqlIndVarSeq.add(nullable);
        if( sqlType == PSTGRSCLI_UUID  )sqlForBitSeq.add(1);
        else sqlForBitSeq.add(0);

        if( sqlType == SQLSRV_BIT  )sqlBitSeq.add(1);
        else sqlBitSeq.add(0);

        //(N)VArchar(MAX): Size is 0 in TDS 7.1/8.0
#ifdef USE_TDS71
        deb("::getColInfo, using TDS71");
        if(sqlType == SQLSRV_NVARCHAR && size == 0 ) sqlVarLengthSeq.add(32000);
        else sqlVarLengthSeq.add(size);
#else
        deb("::getColInfo, using TDS72");
        if(sqlType == SQLSRV_NVARCHAR ) sqlVarLengthSeq.add(32000);
        else sqlVarLengthSeq.add(size);
#endif
        deb("::getColInfo, sqlVarLengthSeq: "+GString(sqlVarLengthSeq.numberOfElements())+" elements");
        if(sqlType == PSTGRSCLI_XML ) xmlSeq.add(1);
        else xmlSeq.add(0);

        if( (sqlType == PSTGRSCLI_BYTEA)  && (size > 8000 || size == 0) ) lobSeq.add(1); //BLOB
        //else if(sqlType == SQLSRV_CLOB && (size > 8000 || size == 0) ) lobSeq.add(2); //CLOB
        else lobSeq.add(0);

        sqlLongTypeSeq.add(0);


        deb("Col "+GString(i)+": Name: "+GString(colName)+", type: "+GString(sqlType)+", colSize: "+GString(colSize)+", varSize: "+GString((int)size));

        /* get display lenght for column */
        //SQLColAttributes (hstmt, i, SQL_COLUMN_DISPLAY_SIZE, NULL, 0,  NULL, &displaysize);

        /* set column length to max of display length, and column name
           length.  Plus one byte for null terminator       */
        //collen[i] = max(displaysize, strlen((char *) colname) ) + 1;


        /* allocate memory to bind column                             */
        //data[i] = (SQLCHAR *) malloc (size);

        /* bind columns to program vars, converting all types to CHAR */
        //SQLBindCol (hstmt, i, SQL_CHAR, data[i], size,  &outlen[i]);
    }
    deb("::getColInfo done");
    return 0;

}

int pgsqlCLI::commit()
{
    //m_db.commit();
    return 0;
}

int pgsqlCLI::rollback()
{
    //m_db.rollback();
    return 0;
}


int pgsqlCLI::initRowCrs()
{
    deb("::initRowCrs");
    if( allRowsSeq.numberOfElements() == 0 ) return 1;
    m_pRowAtCrs = allRowsSeq.initCrs();
    return 0;
}
int pgsqlCLI::nextRowCrs()
{
    if( m_pRowAtCrs == NULL ) return 1;
    m_pRowAtCrs = allRowsSeq.setCrsToNext();
    return 0;
}
long  pgsqlCLI::dataLen(const short & pos)
{
    if( pos < 1 || pos > (short) sqlLenSeq.numberOfElements()) return 0;
    deb("Len at "+GString(pos)+": "+GString(sqlLenSeq.elementAtPosition(pos)));
    return sqlLenSeq.elementAtPosition(pos);
}
int pgsqlCLI::isDateTime(int pos)
{
    if( pos < 1 || pos > (int)sqlTypeSeq.numberOfElements() ) return 0;
    if( sqlTypeSeq.elementAtPosition(pos) == SQL_TYPE_DATE ) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQL_TYPE_TIME ) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQL_TYPE_TIMESTAMP ) return 1;
    return 0;
}

int pgsqlCLI::isNumType(int pos)
{
    if( pos < 1 || pos > (int) sqlTypeSeq.numberOfElements() ) return 0;
    if( sqlTypeSeq.elementAtPosition(pos) == SQL_DECIMAL ) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQL_NUMERIC) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQL_SMALLINT) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQL_INTEGER) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQL_REAL) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQL_FLOAT) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQL_DOUBLE) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQLSRV_BIT) return 1;
    else if( sqlTypeSeq.elementAtPosition(pos) == SQLSRV_BIGINT) return 1;
    return 0;
}
int pgsqlCLI::hasForBitData()
{
    return 0;
}
int pgsqlCLI::isForBitCol(int i)
{
    if( i < 1 || (unsigned long)i > sqlForBitSeq.numberOfElements() ) return 0;
    return sqlForBitSeq.elementAtPosition(i);
}
int pgsqlCLI::isBitCol(int i)
{
    if( i < 1 || (unsigned long)i > sqlBitSeq.numberOfElements() ) return 0;
    return sqlBitSeq.elementAtPosition(i);
}

unsigned int pgsqlCLI::numberOfColumns()
{
    //deb("::numberOfColumns called");
    if( m_SQLHDBC == SQL_NULL_HDBC ) return 0;
    return m_iNumberOfColumns;
}

unsigned long pgsqlCLI::numberOfRows()
{
    if( m_SQLHDBC == SQL_NULL_HDBC ) return 0;
    return m_iNumberOfRows;
}
GString pgsqlCLI::dataAtCrs(int col)
{
    if( m_pRowAtCrs == NULL ) return "@CrsNotOpen";
    if( col < 1 || (unsigned long)col > m_pRowAtCrs->elements() ) return "@OutOfReach";
    return m_pRowAtCrs->rowElementData(col);
}
GString pgsqlCLI::rowElement( unsigned long row, int col)
{
    //Overload 1
    //tm("::rowElement for "+GString(line)+", col: "+GString(col));
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return "@OutOfReach";
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    if( row < 1 || (unsigned long) col > aRow->elements() ) return "OutOfReach";
    return aRow->rowElementData(col);
}
int pgsqlCLI::positionOfHostVar(const GString& hostVar)
{
    unsigned long i;
    for( i = 1; i <= hostVarSeq.numberOfElements(); ++i )
    {
        if( hostVarSeq.elementAtPosition(i) == hostVar ) return i;
    }
    return 0;
}
GString pgsqlCLI::rowElement( unsigned long row, GString hostVar)
{
    //Overload 2
    unsigned long pos;
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return "@OutOfReach";
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    pos = positionOfHostVar(hostVar);
    if( pos < 1 || pos > aRow->elements() ) return "OutOfReach";
    return aRow->rowElementData(pos);
}


GString pgsqlCLI::hostVariable(int col)
{
    //deb("::hostVariable called, col: "+GString(col));
    if( col < 1 || col > (short) hostVarSeq.numberOfElements() ) return "@hostVariable:OutOfReach";
    return hostVarSeq.elementAtPosition(col);
}



GString pgsqlCLI::currentCursor(GString filter, GString command, long curPos, short commitIt, GSeq <GString> *fileList, GSeq <long> *lobType)
{
    deb(__FUNCTION__, "filter: "+filter+", cmd: "+command);
    int rc;
    if( curPos < 0 ) return "CurrentCursor(): POS < 0";
    SQLHSTMT     hstmtSelect;
    SQLHSTMT     hstmtUpdate;
    SQLRETURN    retcode;


    rc = SQLAllocHandle(SQL_HANDLE_STMT, m_SQLHDBC, &hstmtSelect);
    rc = SQLSetStmtAttr(hstmtSelect, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0); //<-Important
    rc = SQLAllocHandle(SQL_HANDLE_STMT, m_SQLHDBC, &hstmtUpdate);
    rc = SQLSetCursorName(hstmtSelect, (SQLCHAR*)"PMF_UPD_CRS", SQL_NTS);
    rc = SQLSetStmtAttr(hstmtSelect,SQL_ATTR_CONCURRENCY,(SQLPOINTER) SQL_CONCUR_LOCK,0);//<-Also important

    rc = SQLExecDirect(hstmtSelect,(SQLCHAR*) filter, SQL_NTS);
    if(rc)
    {
        sqlError(hstmtSelect);
        m_iLastSqlCode = rc;
        return rc;
    }

    long count = 0;
    command += " WHERE CURRENT OF PMF_UPD_CRS";
    do
    {
        retcode = SQLFetch(hstmtSelect);
        count++;
        deb(__FUNCTION__, "retcode: "+GString(retcode));
        if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
        {
            if( count != curPos ) continue;

            if( fileList != NULL )
            {
                deb(__FUNCTION__, "FileList is valid, elements: "+GString(fileList->numberOfElements()));
                if( fileList->numberOfElements() ) rc = uploadBlob(command, fileList, lobType);
            }
            else rc = SQLExecDirect(hstmtUpdate, (SQLCHAR*)command, SQL_NTS);
            deb(__FUNCTION__, "rcUpd: "+GString(rc));
            if( rc ) sqlError(hstmtUpdate);
            else m_iLastSqlCode = 0;
        }
        break;
    }
    while ((retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) );
    //SQLCloseCursor(hstmtSelect);
    if( commitIt )  SQLEndTran(SQL_HANDLE_DBC,m_SQLHDBC,SQL_COMMIT);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmtSelect);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmtUpdate);
    //freeStmt(hstmtSelect);
    //freeStmt(hstmtUpdate);

    deb(__FUNCTION__, "done, rc: "+GString(rc));
    return rc ? m_strLastError : GString("");
}



int pgsqlCLI::sqlCode()
{
    //deb("::sqlCode: m_iLastSqlCode = "+GString(m_iLastSqlCode));
    return m_iLastSqlCode;
}
GString pgsqlCLI::sqlError()
{
    deb("::sqlError: m_iLastSqlCode = "+GString(m_iLastSqlCode));
    if( m_iLastSqlCode == 100 ) return "SQLCode 100: No row(s) found.";
    sqlError(m_SQLHSTMT);
    return m_strLastError;
}
void pgsqlCLI::errorState(SQLHSTMT hstmt) {
    unsigned char szSQLSTATE[10];
    SDWORD nErr;
    unsigned char msg[SQL_MAX_MESSAGE_LENGTH + 1];
    SWORD cbmsg;
    deb("error_out start");
    int rc = SQLError(0, 0, hstmt, szSQLSTATE, &nErr, msg, sizeof(msg), &cbmsg);
    deb("RC: "+GString(rc));
    //printf("Err_out: State: %s, Nr: %i, msg: %s\n", szSQLSTATE, nErr, msg);
    while (rc == SQL_SUCCESS) {
        // printf("Err_out: State: %s, Nr: %i, msg: %s\n", szSQLSTATE, nErr, msg);
    }
    deb("error_out end");
}

GString pgsqlCLI::sqlError(SQLHSTMT stmt)
{
    deb("::sqlError() start");
    if( stmt == 0 )stmt = m_SQLHSTMT;

    if( m_SQLHDBC == SQL_NULL_HDBC ) return "";
    GString err;

    SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER    NativeError = 0;
    SQLSMALLINT   i, MsgLen;
    i = 1;

    int ret1 = SQLGetDiagRec(SQL_HANDLE_DBC, m_SQLHDBC, i, SqlState, &NativeError,  Msg, sizeof(Msg), &MsgLen);
    if( ret1 == SQL_SUCCESS )
    {
        err = GString(SqlState)+": "+GString(NativeError)+": "+GString(Msg);
        deb("::sqlError() SQLGetDiagRec/SQL_HANDLE_DBC: "+err);
        //if( NativeError ) m_iLastSqlCode = NativeError;
    }
    int ret2 = SQLGetDiagRec(SQL_HANDLE_STMT, stmt, i, SqlState, &NativeError,  Msg, sizeof(Msg), &MsgLen);
    if( ret2 == SQL_SUCCESS )
    {
        deb("::sqlError() SQLGetDiagRec/SQL_HANDLE_STMT SqlState: "+GString(SqlState));
        deb("::sqlError() SQLGetDiagRec/SQL_HANDLE_STMT NativeErr: "+GString(NativeError));
        deb("::sqlError() SQLGetDiagRec/SQL_HANDLE_STMT Msg: "+GString(Msg));
        err += GString(SqlState)+": "+GString(NativeError)+": "+GString(Msg);

        deb("::sqlError() SQLGetDiagRec/SQL_HANDLE_STMT: "+err);
        //if( NativeError ) m_iLastSqlCode = NativeError;
    }

    if( ret1 && ret2 )
    {
        //m_iLastSqlCode = 0;
        //m_strLastError = "";
    }
    else
    {	//NativeError appears to be more or less useless, and SqlState is a 5-char string.
        m_iLastSqlCode = GString(SqlState).asInt();
        //if( err.length() && m_iLastSqlCode ) m_strLastError = err;
        if( err.length() ) m_strLastError = err;
    }

    deb("::sqlError() final: SqlCode: "+GString(m_iLastSqlCode)+", err: "+m_strLastError);

    return m_strLastError;
}


int pgsqlCLI::getTabSchema()
{
    GString cmd = "SELECT DISTINCT(nspname) FROM pg_catalog.pg_namespace ORDER BY nspname";
    this->initAll(cmd);
    return 0;
}

int pgsqlCLI::getTables(GString schema)
{
    GString cmd = "SELECT table_name FROM information_schema.tables where table_schema='"+schema+"' and table_catalog='"+m_strCurrentDatabase+"' order by table_name";
     this->initAll(cmd);
     return 0;
}

void pgsqlCLI::convToSQL( GString& input )
{

    GStuff::convToSQL(input);
    return;
}

int pgsqlCLI::getAllTables(GSeq <GString > * tabSeq, GString filter)
{
    PMF_UNUSED(tabSeq);
    PMF_UNUSED(filter);
    /*
QStringList sl = m_db.tables(QSql::AllTables);
tabSeq->removeAll();
for (int i = 0; i < sl.size(); ++i) tabSeq->add(sl.at(i));
*/
    return 0;
}
short pgsqlCLI::sqlType(const short & col)
{
    if( col < 1 || col > (short) sqlTypeSeq.numberOfElements() ) return -1;
    else return sqlTypeSeq.elementAtPosition(col);
}
short pgsqlCLI::sqlType(const GString & colName)
{
    for( short i = 1; i <= (short) hostVarSeq.numberOfElements(); ++ i )
    {
       if( hostVarSeq.elementAtPosition(i) == GString(colName).strip("\"") )
       {
           return sqlTypeSeq.elementAtPosition(i);
       }
    }
    return -1;
}
GString pgsqlCLI::realName(const short & sqlType)
{
    switch(sqlType)
    {
            return "Numeric";

            return "Time";

        default:
            return "'string'";
    }
    return "";
}
int pgsqlCLI::loadFileIntoBufNoConvert(GString fileName, char** fileBuf, int *size)
{
    FILE * f;
    //Apparently CR/LF is not counted in fread.
    //Even XML sources should be opened with "rb".
    f = fopen(fileName, "rb");
    if( f != NULL )
    {
        char* fb;
        int sz;

        deb("::loadFileIntoBuf reading file....");
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        fb = new char[sz+1];
        fread(fb, 1, sz, f);
        fclose(f);
        fb[sz] = '\0';
        deb("::loadFileIntoBuf reading file...OK, size: "+GString(sz));
        *size = sz;
        *fileBuf = fb;
        return 0;
    }
    return 1;
}

int pgsqlCLI::loadFileIntoBuf(GString fileName, char** fileBuf, int *size)
{

    FILE * f;
    //Apparently CR/LF is not counted in fread.
    //Even XML sources should be opened with "rb".
    f = fopen(fileName, "rb");
    if( f != NULL )
    {
        char* fb;
        int sz;
        deb("::loadFileIntoBuf reading file....");
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        fb = new char[sz+1];
        int res = fread(fb,sizeof(char),sz, f);
        fclose(f);
        deb("::trying to encode....");
        std::string enc = GStuff::base64_encode((unsigned char*) fb, sz);
        deb("::trying to encode....Done.");
        delete [] fb;
        sz = enc.size();
        fb = new char[sz+1];
        strncpy(fb, (char*)GString(enc), sz);
        fb[sz] = '\0';

        *size = sz;
        *fileBuf = fb;
        deb("::loadFileIntoBuf reading file...OK, encoded size: "+GString(*size)+", bytesRead from orig. file: "+GString(res)+", GString(enc): "+GString(GString(enc).length()));

        /*
        FILE * fOut;
        fOut = fopen("/home/moi/develop/c/pmf6/decoded.bin", "ab");
        std::string ret = GStuff::base64_decode((char*) fb);
        fwrite((char*) ret.c_str(),1,ret.size(),fOut);
        fclose(fOut);
        */
        return 0;
    }
    return 1;
}
void pgsqlCLI::impExpLob()
{
//    Oid         lobjOid;

//    m_pRes = PQexec(m_pgConn, "begin");
//    PQclear(m_pRes);

//    lobjOid = lo_import(m_pgConn, "/home/moi/tok.txt");

//    lo_export(m_pgConn, lobjOid, "/home/moi/tok3.txt");

//    m_pRes = PQexec(m_pgConn, "end");
//    PQclear(m_pRes);
}

GString pgsqlCLI::allPurposeFunction(GKeyVal *pKeyVal)
{
    //GString s =  (*pKeyVal)[1]->key;
//    if( pKeyVal->hasKey("UNLINK_BLOB"))
//    {
//        GString oid = pKeyVal->getValForKey("UNLINK_BLOB");
//        m_pRes = PQexec(m_pgConn, "begin");
//        PQclear(m_pRes);
//        lo_unlink(m_pgConn, oid.asInt());
//        m_pRes = PQexec(m_pgConn, "end");
//        return sqlError();
//    }
    if( pKeyVal->hasKey("EXPORT_TO_CSV"))
    {
        GString cmd = pKeyVal->getValForKey("SQLCMD");
        GString trgFile = pKeyVal->getValForKey("TARGET_FILE");
        GString delim = pKeyVal->getValForKey("DELIM");
        int wrHead = pKeyVal->getValForKey("WRITE_HEADER").asInt();
        int byteaAsFile = pKeyVal->getValForKey("BYTEA_AS_FILE").asInt();
        int xmlAsFile = pKeyVal->getValForKey("XML_AS_FILE").asInt();
        int exportLobs = pKeyVal->getValForKey("EXPORT_LOBS").asInt();
        return exportCsvBytea(cmd, trgFile, delim, wrHead, byteaAsFile, xmlAsFile, exportLobs);
    }
    return "";
}
GString pgsqlCLI::exportCsvBytea(GString message, GString targetFile, GString delim, int writeHeader, int byteaAsFile, int xmlAsFile, int exportLobs)
{
    return "";
//    deb("::exportByteArrayToFile, start. msg: "+message);


//    GFile trgFile(targetFile, GF_OVERWRITE);
//    m_pRes = PQexec(m_pgConn, message);
//    if (PQresultStatus(m_pRes) == PGRES_COMMAND_OK) return "";
//    else
//    {
//        sqlError();
//        if(m_strLastError.length()) return m_strLastError;
//    }
//    m_pRes = PQexec(m_pgConn, "BEGIN");
//    if (PQresultStatus(m_pRes) != PGRES_COMMAND_OK)
//    {
//        return sqlError();
//    }

//    PQclear(m_pRes);

//    message = "DECLARE myCRS CURSOR FOR "+message;
//    m_pRes = PQexec(m_pgConn, message);
//    if (PQresultStatus(m_pRes) != PGRES_COMMAND_OK)
//    {
//        sqlError();
//        m_pRes = PQexec(m_pgConn, "END");
//        PQclear(m_pRes);
//        return m_strLastError;
//    }
//    PQclear(m_pRes);

//    m_pRes = PQexec(m_pgConn, "FETCH ALL in myCRS");
//    if (PQresultStatus(m_pRes) != PGRES_TUPLES_OK)
//    {
//        return sqlError();
//    }
//    deb("cols: "+GString(m_iNumberOfColumns));
//    int colCount = PQnfields(m_pRes);
//    GString out;

//    //Header
//    for (int i = 0; i < colCount; i++)
//    {
//        out += GString(PQfname(m_pRes, i)).strip("\"")+delim;
//    }
//    out = out.stripTrailing(delim);
//    if( writeHeader ) trgFile.addLine(out);

//    //Rows
//    GString data;
//    out = "";
//    for (int i = 0; i < PQntuples(m_pRes); i++)
//    {
//        out = "";
//        for (int j = 0; j < colCount; j++)
//        {
//            if( PQgetisnull(m_pRes, i, j) )data = "NULL";
//            else if( isNumType(j+1) ) data = PQgetvalue(m_pRes, i, j);
//            else if ( isXMLCol(j+1) && xmlAsFile == 0 )
//            {
//               data = "'"+GString(PQgetvalue(m_pRes, i, j)).removeAll('\n')+"'";
//            }
//            else if( (isLOBCol(j+1) && byteaAsFile && exportLobs ) || isXMLCol(j+1) )
//            {
//                GFile ba_file(targetFile+"_PMF_"+GString(i)+":"+GString(j));
//                data = "'"+GString(PQgetvalue(m_pRes, i, j))+"'";
//                ba_file.addLine(data);
//                data = targetFile+"_PMF_"+GString(i)+":"+GString(j);
//            }
//            else if( isLOBCol(j+1) && !exportLobs ) data ="NULL";
//            else data = "'"+GString(PQgetvalue(m_pRes, i, j))+"'";
//            out += data + delim;
//        }
//        out = out.stripTrailing(delim);
//        trgFile.addLine(out);
//    }

//    PQclear(m_pRes);
//    m_pRes = PQexec(m_pgConn, "CLOSE myCRS");
//    PQclear(m_pRes);

//    m_pRes = PQexec(m_pgConn, "END");
//    return sqlError();
}

GString pgsqlCLI::exportBlob(unsigned int oid, GString target)
{
//    m_pRes = PQexec(m_pgConn, "begin");
//    PQclear(m_pRes);
//    int erc = lo_export(m_pgConn, oid, target);
//    m_pRes = PQexec(m_pgConn, "end");
//    if( erc == 1 ) return "";
//    return sqlError();
    return "";
}

long pgsqlCLI::importBlob(GSeq <GString> *fileSeq)
{
//    if( fileSeq->numberOfElements() == 0 ) return -1;
//    Oid         lobjOid;
//    m_pRes = PQexec(m_pgConn, "begin");
//    PQclear(m_pRes);
//    lobjOid = lo_import(m_pgConn, fileSeq->elementAtPosition(1));
//    m_pRes = PQexec(m_pgConn, "end");
//    PQclear(m_pRes);
//    return lobjOid;
    return 0;
}


/********************************************************************
* lobTypeSequence is -4 for BLOBs, -1 for CLOBs, and 0 else
*/

long pgsqlCLI::uploadBlob(GString cmd, GSeq <GString> *fileSeq, GSeq <long> *lobType)
{
    deb("::uploadBlob, cmd: "+cmd);
    SQLRETURN       ret = 0;

    m_iLastSqlCode = 0;

    if(fileSeq->numberOfElements() == 0 ) return 0;

    unsigned char ** fileBuf = new unsigned char* [fileSeq->numberOfElements()];
    //char ** fileBuf = new char* [fileSeq->numberOfElements()];
    SQLLEN*         size = new SQLLEN[fileSeq->numberOfElements()];

    deb("::uploadBlob, SQLAllocHandle, rc: "+GString(ret));

    if( m_SQLHSTMT) SQLFreeHandle(SQL_HANDLE_STMT, m_SQLHSTMT);
    SQLAllocHandle(SQL_HANDLE_STMT, m_SQLHDBC, &m_SQLHSTMT);

    SQLPrepare(m_SQLHSTMT, (SQLCHAR*)cmd, SQL_NTS);

//    for(unsigned long i = 1; i <= fileSeq->numberOfElements(); ++i )
//    {
//        deb("::uploadBlob, try open "+fileSeq->elementAtPosition(i));
//        if( lobType->elementAtPosition(i) == SQLSRV_BLOB || lobType->elementAtPosition(i) == SQLSRV_IMAGE ) //Blob
//        {
//            deb("::uploadBlob: Is binary: "+GString(lobType->elementAtPosition(i)));
//            if( !loadFileIntoBuf(fileSeq->elementAtPosition(i), &(fileBuf[i-1]), &(size[i-1])) )
//            {
//                ret = SQLBindParameter(m_SQLHSTMT, i, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, size[i-1], 0, (SQLCHAR*)fileBuf[i-1], size[i-1], &size[i-1]) ;
//            }
//        }
//        else
//        {
//            deb("::uploadBlob: Is char: "+GString(lobType->elementAtPosition(i)));
//            if( !loadFileIntoBuf(fileSeq->elementAtPosition(i), &(fileBuf[i-1]), &(size[i-1]), 0) )
//            {
//                ret = SQLBindParameter(m_SQLHSTMT, i, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR, size[i-1], 0, (SQLCHAR*)fileBuf[i-1], size[i-1], &size[i-1]) ;
//            }
//        }
//    }
    if( !ret )  ret = SQLExecute(m_SQLHSTMT);

    delete [] fileBuf;
    delete [] size;
    deb("::uploadBlob, SQLExecute, rc: "+GString(ret)+", err: "+sqlError());

    freeStmt(m_SQLHSTMT);
    m_iLastSqlCode = ret;
    return ret;
}
void pgsqlCLI::freeStmt(SQLHSTMT stmt)
{
    m_strLastError = sqlError(stmt);
    SQLFreeStmt(stmt, SQL_CLOSE);
}

/*

long pgsqlCLI::uploadBlob(GString cmd, GSeq <GString> *fileSeq, GSeq <long> *lobType)
{
deb("::uploadBlob, cmd: "+cmd);
SQLRETURN       ret = 0;
SQLLEN         size;
unsigned char * fileBuf;

deb("::uploadBlob, SQLAllocHandle, rc: "+GString(ret));

if( m_SQLHSTMT) SQLFreeHandle(SQL_HANDLE_STMT, m_SQLHSTMT);
SQLAllocHandle(SQL_HANDLE_STMT, m_SQLHDBC, &m_SQLHSTMT);
SQLPrepare(m_SQLHSTMT, (SQLCHAR*)cmd, SQL_NTS);

for(int i = 1; i <= fileSeq->numberOfElements(); ++i )
{
    deb("::uploadBlob, try open "+fileSeq->elementAtPosition(i));
    loadFileIntoBuf(fileSeq->elementAtPosition(i), &fileBuf, &size);
    ret = SQLBindParameter(m_SQLHSTMT, i, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, size, 0, (SQLCHAR*)fileBuf, size, &size);
}

ret = SQLExecute(m_SQLHSTMT);

delete [] fileBuf;
//ret = SQLExecDirect(m_SQLHSTMT, (SQLCHAR*)cmd, SQL_NTS);
deb("::uploadBlob, SQLExecute, rc: "+GString(ret)+", err: "+sqlError());

if(ret) return sqlError();


SQLFreeHandle(SQL_HANDLE_STMT, m_SQLHSTMT);
SQLFreeStmt(m_SQLHSTMT, SQL_CLOSE);
return sqlError();

}
*/
GString pgsqlCLI::descriptorToFile( GString cmd, GString &blobFile, int * outSize )
{
    deb("::descriptorToFile, cmd: "+cmd);
    if( m_SQLHDBC == SQL_NULL_HDBC )
    {
        deb("::descriptorToFile: No handle, quitting.");
        return "";
    }

    SQLRETURN       ret;
    SQLSMALLINT     columns;
    SQLCHAR         colName[128];
    SQLSMALLINT     sqlType;
    SQLSMALLINT     colSize;
    SQLSMALLINT     nullable;
    SQLULEN         size;
    SQLSMALLINT     scale;


    m_strLastError = "";
    m_iLastSqlCode = 0;

    SQLFreeHandle(SQL_HANDLE_DBC, m_SQLHDBC);
    ret = SQLAllocHandle(SQL_HANDLE_STMT, m_SQLHDBC, &m_SQLHSTMT);
    deb("::descriptorToFile, SQLAllocHandle, rc: "+GString(ret));
    if(ret)
    {
        m_iLastSqlCode = ret;
        return sqlError();
    }

    ret = SQLExecDirect(m_SQLHSTMT, (SQLCHAR*)cmd, SQL_NTS);
    deb("::descriptorToFile, SQLExecDirect, rc: "+GString(ret));
    if(ret)
    {
        m_iLastSqlCode = ret;
        return sqlError();
    }


    ret = SQLNumResultCols(m_SQLHSTMT, &columns);
    deb("::descriptorToFile, SQLNumResultCols, rc: "+GString(ret)+", cols: "+GString(columns));
    if(ret)
    {
        m_iLastSqlCode = ret;
        return sqlError();
    }


    SQLLEN rowCount;
    SQLRowCount(m_SQLHSTMT, &rowCount);
    if( !rowCount )
    {
        m_iLastSqlCode = 100;
        return "The current row was changed (possibly by another application or a trigger).\nSimply refresh the view (hit F5 or click 'Open').";
    }


    char *buf;
    int bufLen = 10000000; //Read 10MB Blocks
    buf = new char[bufLen+1];


    while (SQL_SUCCEEDED(ret = SQLFetch(m_SQLHSTMT)))
    {
        SQLLEN len = 0;

        for ( int i = 1; i <= columns; i++ )
        {
            *outSize = 0;
            remove(blobFile);
            SQLDescribeCol (m_SQLHSTMT, i, colName, sizeof (colName),  &colSize, &sqlType, &size, &scale, &nullable);
            if(sqlType != SQLSRV_XML && sqlType != SQLSRV_BLOB && sqlType != SQLSRV_CLOB && sqlType != SQLSRV_IMAGE )
            {
                freeStmt(m_SQLHSTMT);
                return "Invalid column type";
            }
            while( 1 )
            {
                //memset(buf, 0, bufLen);
                memset(buf, 0, bufLen);
                if( sqlType == SQLSRV_XML )ret = SQLGetData( m_SQLHSTMT, i, SQL_C_CHAR, (SQLPOINTER) ((char *) (buf)),(SQLINTEGER) bufLen,(SQLLEN *) &len );
                else ret = SQLGetData( m_SQLHSTMT, i, SQL_C_BINARY, (SQLPOINTER) ((char *) (buf)),(SQLINTEGER) bufLen,(SQLLEN *) &len );

                deb("descriptorToFile, col: "+GString(i)+", type: "+GString(sqlType)+", bufLen: "+GString(bufLen)+", size: "+GString((int)len)+", ret: "+GString(ret));

                if( len < bufLen ) bufLen = len;
                *outSize += writeToFile(blobFile, &buf, bufLen);
                deb("written: "+GString(*outSize)+", file: "+blobFile);
                if( ret == 0 || ret == 100 ) break;
            }
            /*
        deb("descriptorToFile, col: "+GString(i)+", type: "+GString(sqlType)+", size: "+GString(size));
        if( size == 0 )
        {
            //This is a workaround for getting LOBs on WIN. The size of LOB columns is returned as 0. We call
            //SQLGetData() with empty buffers and DO NOT FETCH A SINGLE BYTE. Instead, we get the actual size
            //of the LOB in the cell.
            buf = new char[0];
            ret = SQLGetData( m_SQLHSTMT, i, SQL_C_BINARY, (SQLPOINTER) ((char *) (buf)),(SQLINTEGER) 0,(SQLLEN *) &len );
            deb("descriptorToFile2, col: "+GString(i)+", type: "+GString(sqlType)+", size: "+GString(size)+", len: "+GString(len));
            size = len-1;
            delete [] buf;
        }
        bufLen = size+1;
        buf = new char[bufLen];
        ///ret = SQLGetData(m_SQLHSTMT, i, SQL_C_CHAR, buf, bufLen, &len);
        ///SQL_C_BINARY
        //ret = SQLGetData(m_SQLHSTMT, i, SQL_C_BINARY, buf, bufLen, &len);
        ret = SQLGetData( m_SQLHSTMT, i, SQL_C_BINARY, (SQLPOINTER) ((char *) (buf)),(SQLINTEGER) bufLen,(SQLLEN *) &len );
        deb("descriptorToFile,ret: "+GString(ret)+", col: "+GString(i)+", bufLen: "+GString(bufLen)+", len: "+GString(len));
        //if (!ret)
        {
            FILE * f;
            if( (f = fopen(blobFile, "wb")) != NULL )
            {
                //fputs(buf, f);
                fwrite(buf,1,len,f);
                fclose(f);
                deb("::descriptorToFile, ret: "+GString(ret)+", out: "+blobFile+", len: "+GString(len)+", bufLen: "+GString(bufLen));
            }
            else remove(blobFile);
        }
        delete[] buf;
        */
        }
    }
    *outSize = bufLen;
    freeStmt(m_SQLHSTMT);

    return m_strLastError;
}
int pgsqlCLI::writeToFile(GString fileName, char** buf, int len)
{
    FILE * f;
    f = fopen(fileName, "ab");
    int written = fwrite(*buf,1,len,f);
    fclose(f);
    return written;
}
signed long pgsqlCLI::getCost()
{
    return -1;
}
int pgsqlCLI::isXMLCol(int pos)
{
    if( pos < 1 || pos > (int) sqlTypeSeq.numberOfElements() ) return 0;
    if( sqlTypeSeq.elementAtPosition(pos) == PSTGRSCLI_XML ) return 1;
    else return 0;
}
int pgsqlCLI::simpleColType(int i)
{
    if( i < 1 || (unsigned long)i > sqlTypeSeq.numberOfElements() ) return CT_UNKNOWN;

     if(isXMLCol(i)) return CT_XML;
     if(isLOBCol(i)) return CT_BLOB;
     if(isNumType(i)) return CT_INTEGER;
     if(isDateTime(i)) return CT_DATE;
     return CT_STRING;}

int pgsqlCLI::isLOBCol(int i)
{
    if( i < 1 || (unsigned long)i > sqlTypeSeq.numberOfElements() ) return 0;
     switch(sqlTypeSeq.elementAtPosition(i))
     {
     case PSTGRSCLI_BYTEA:
         return 1;

     }
     return 0;
}

int pgsqlCLI::isNullable(int i)
{
    if( i < 1 || (unsigned long)i > sqlIndVarSeq.numberOfElements() ) return 1;
    return sqlIndVarSeq.elementAtPosition(i);
}

int pgsqlCLI::isFixedChar(int i)
{
    return 0;
}

int pgsqlCLI::getDataBases(GSeq <CON_SET*> *dbList)
{
    CON_SET * pCS;
    /*
GString err = this->initAll("SELECT NAME FROM SYS.DATABASES ORDER BY NAME");
deb("getDataBases: "+err);
for(unsigned long i = 1; i <= this->numberOfRows(); ++i)
{
    pCS = new CON_SET;
    pCS->DB = this->rowElement(i, 1);
    pCS->Host = _HOST_DEFAULT;
    dbList->add(pCS);
}
*/
    deb("getDataBases start");
    SQLHENV env;
    char dsn[256];
    char desc[256];
    SQLSMALLINT dsn_ret;
    SQLSMALLINT desc_ret;
    SQLUSMALLINT direction;
    SQLRETURN ret;

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

    direction = SQL_FETCH_FIRST;
    while(SQL_SUCCEEDED(ret = SQLDataSources(env, direction,
                                             (SQLCHAR*)dsn, sizeof(dsn), &dsn_ret,
                                             (SQLCHAR*)desc, sizeof(desc), &desc_ret)))
    {
        direction = SQL_FETCH_NEXT;
        deb("getDataBases, got DESC: "+GString(desc)+", DSN: "+GString(dsn));
#ifdef MAKE_VC
        if( GString(desc).occurrencesOf("SQL") && GString(desc).occurrencesOf("Server") )
#endif
        {
            pCS = new CON_SET;
            pCS->DB = dsn;
            deb("getDataBases, adding "+GString(dsn));
            pCS->Host = _HOST_DEFAULT;
            pCS->Type = _PGSQLCLI;
            dbList->add(pCS);
        }
    }
    deb("getDataBases done");
    return ret == 100 ? 0 : ret;
}
void pgsqlCLI::setCurrentDatabase(GString db)
{
    m_strCurrentDatabase = db;
}
GString pgsqlCLI::currentDatabase()
{
    return m_strCurrentDatabase;
}

/*****************************************************************************
*
* PRIVATE METHOD
*
*****************************************************************************/
void pgsqlCLI::resetAllSeq()
{
    GRowHdl * aLine;
    hostVarSeq.removeAll();
    sqlTypeSeq.removeAll();
    sqlVarLengthSeq.removeAll();
    sqlIndVarSeq.removeAll();
    sqlForBitSeq.removeAll();
    sqlBitSeq.removeAll();
    xmlSeq.removeAll();
    lobSeq.removeAll();
    sqlLenSeq.removeAll();
    simpleColTypeSeq.removeAll();
    m_iNumberOfRows   = 0;
    m_iNumberOfColumns = 0;

    while( !allRowsSeq.isEmpty() )
    {
        aLine = allRowsSeq.firstElement();
        //aLine->deleteLine();
        delete aLine;
        allRowsSeq.removeFirst();
    }
}
int pgsqlCLI::getColSpecs(GString table, GSeq<COL_SPEC*> *specSeq)
{
    GString tabSchema = this->tabSchema(table, 1);
    GString tabName   = this->tabName(table, 1);
    COL_SPEC *cSpec;

    this->setCLOBReader(1); //This will read any CLOB into fieldData

    GString cmd = "SELECT column_name, data_type, character_maximum_length, numeric_precision, numeric_scale, is_nullable, column_default, is_identity, "
                  "identity_generation, identity_start, identity_increment, identity_maximum, identity_minimum, identity_cycle, is_updatable "
                  "FROM information_schema.COLUMNS WHERE TABLE_SCHEMA='"+tabSchema+"' AND table_name='"+tabName+"'";
    deb("::getColSpecs: "+cmd);

    this->initAll(cmd);




    for( unsigned i=1; i<=this->numberOfRows(); ++i )
    {
        cSpec = new COL_SPEC;
        cSpec->init();

        cSpec->ColName = this->rowElement(i,1).strip("'");

        /* colType CHARACTER (from syscat.tables) needs to be truncated to CHAR */
        cSpec->ColType = this->rowElement(i,2).strip("'");
        if( cSpec->ColType == "CHARACTER" ) cSpec->ColType = "CHAR";


        /* SMALLINT, INTEGER, BIGINT, DOUBLE */
        if( cSpec->ColType.occurrencesOf("int") > 0 ||
                cSpec->ColType.occurrencesOf("double") > 0 ||
                cSpec->ColType.occurrencesOf("float") > 0 ||
                cSpec->ColType.occurrencesOf("date") > 0 ||
                cSpec->ColType.occurrencesOf("serial") > 0 ||
                cSpec->ColType.occurrencesOf("box") > 0 ||
                cSpec->ColType.occurrencesOf("boolean") > 0 ||
                cSpec->ColType.occurrencesOf("json") > 0 ||
                cSpec->ColType.occurrencesOf("line") > 0 ||
                cSpec->ColType.occurrencesOf("time"))
        {
            cSpec->Length = "N/A";
        }
        /* DECIMAL: (LENGTH, SCALE) */
        else if( cSpec->ColType.occurrencesOf("decimal") || cSpec->ColType.occurrencesOf("numeric") )
        {
            cSpec->Length = this->rowElement(i,3)+", "+this->rowElement(i,4);
        }
        else if( cSpec->ColType.occurrencesOf("xml") || cSpec->ColType.occurrencesOf("bytea") )
        {
            cSpec->Length = "N/A";
        }
        else
        {
            cSpec->Length = this->rowElement(i,3).strip("'");
        }
        if( this->rowElement(i, 8) == "'YES'" )
        {
            cSpec->Identity = "Y";
            cSpec->Misc = "GENERATED "+this->rowElement(i, 9).strip("\'")+" AS IDENTITY (START WITH "+this->rowElement(i, 10).strip("\'")+" INCREMENT BY "+this->rowElement(i, 11).strip("\'")+")";
        }


        /* check for NOT NULL, DEFAULT */
        if( this->rowElement(i,6) == "'NO'" )
        {
            cSpec->Nullable = "NOT NULL";
        }
        if( !this->isNull(i, 7) )
        {
            cSpec->Default = this->rowElement(i,7);
        }
        specSeq->add(cSpec);

    }

    this->setCLOBReader(0);
    return 0;
}

int pgsqlCLI::getIdentityColParams(GString table, int *seed, int * incr)
{
    pgsqlCLI * tmp = new pgsqlCLI(*this);
    tmp->initAll("select IDENT_SEED ( '"+table+"' )");
    *seed = tmp->rowElement(1,1).asInt();

    tmp->initAll("select IDENT_INCR ( '"+table+"' )");
    *incr = tmp->rowElement(1,1).asInt();

    delete tmp  ;
    return 0;
}


int pgsqlCLI::getTriggers(GString table, GString *text)
{
    *text = "";
    GSeq <GString> trgSeq = getTriggerSeq(table);
    for(int i = 1; i <= (int)trgSeq.numberOfElements(); ++i)
    {
        if( trgSeq.elementAtPosition(i).strip().length())
        {
            *text += trgSeq.elementAtPosition(i).change("\n", " ")+"\n";
        }
    }
    return 0;
}

GSeq <GString> pgsqlCLI::getTriggerSeq(GString table)
{
    GSeq <GString> triggerSeq;
    deb("getTriggers for table: "+table);
    setDatabaseContext(context(table));

    while(table.occurrencesOf(".")) table = table.remove(1, table.indexOf("."));
    table = table.change("\"", "'");

    GString cmd = "SELECT Comments.Text FROM sysobjects Triggers "
                  "Inner Join sysobjects Tables On Triggers.parent_obj = Tables.id "
                  "Inner Join syscomments Comments On Triggers.id = Comments.id "
                  "WHERE Triggers.xtype = 'TR' And Tables.xtype = 'U' "
                  "And Tables.Name="+table+" ORDER BY Tables.Name, Triggers.name";
    this->initAll(cmd);
    deb("getTriggers, cmd: "+cmd);
    for( unsigned long i = 1; i <= this->numberOfRows(); ++i )
    {
        if( this->rowElement(i,1).strip("'").strip().length())
        {
            triggerSeq.add(this->rowElement(i,1).strip("'").strip());
        }
    }
    return triggerSeq;
}


GSeq <IDX_INFO*> pgsqlCLI::getIndexeInfo(GString table)
{
    GString id, name, cols, unq, crt, mod, dis;
    int indexID = -1;
    setDatabaseContext(context(table));

    GString cmd = "SELECT I.index_id as [INDEXID], I.[name] AS [NAME],  AC.[name] AS [COLUMN],  "
                  "I.[is_unique] as [UNQRULE], I.[is_primary_Key] as [PRIMKEY], BaseT.create_date as CRTD,"
                  "BaseT.modify_date as MOD, I.is_disabled as ISDIS "
                  "FROM sys.[tables] AS BaseT  "
                  "INNER JOIN sys.[indexes] I ON BaseT.[object_id] = I.[object_id]  "
                  "INNER JOIN sys.[index_columns] IC ON I.[object_id] = IC.[object_id] and I.index_id = IC.index_id "
                  "INNER JOIN sys.[all_columns] AC ON BaseT.[object_id] = AC.[object_id] AND IC.[column_id] = AC.[column_id] "
                  "WHERE BaseT.[is_ms_shipped] = 0 AND I.[type_desc] <> 'HEAP' "
                  "and OBJECT_SCHEMA_NAME(BaseT.[object_id],DB_ID()) = '"+tabSchema(table)+"'"
                                                                                           "and  BaseT.[name] ='"+tabName(table)+"'";
    this->initAll(cmd);
    deb("::getIndexeInfo, cmd: "+cmd);
    deb("::getIndexeInfo, found: "+GString(this->numberOfRows()));

    GSeq <IDX_INFO*> indexSeq;

    IDX_INFO * pIdx;
    for(int  i=1; i<=(int)this->numberOfRows(); ++i )
    {
        pIdx = new IDX_INFO;
        deb("::getIndexeInfo, i: "+GString(i));
        if( this->rowElement(i, "INDEXID").strip("'").asInt() != indexID )
        {
            deb("::getIndexeInfo, i: "+GString(i)+", indexChg, new: "+GString(indexID));
            if( cols.length() )
            {
                cols = cols.stripTrailing(", ");
                pIdx->Iidx = id;
                pIdx->Name = name;
                pIdx->Columns = cols;
                pIdx->Type = unq;
                pIdx->CreateTime = crt;
                pIdx->StatsTime = mod;
                pIdx->IsDisabled = dis;
                indexSeq.add(pIdx);
            }
            id   = this->rowElement(i, "INDEXID");
            name = this->rowElement(i, "NAME");
            cols = "";
            if( this->rowElement(i, "PRIMKEY") == "1" ) unq = DEF_IDX_PRIM;
            else if( this->rowElement(i, "UNQRULE") == "1" ) unq = DEF_IDX_UNQ;
            else unq = DEF_IDX_DUPL;
            crt = this->rowElement(i, "CRTD");
            mod = this->rowElement(i, "MOD");
            dis = this->rowElement(i, "ISDIS");
            indexID = this->rowElement(i, "INDEXID").strip("'").asInt();
            deb("::getIndexeInfo, i: "+GString(i)+", currIDx: "+GString(id)+", cols: "+cols);
        }
        cols += this->rowElement(i, "COLUMN").strip("'")+", ";
        deb("::fillIndexView, i: "+GString(i)+", currIDx: "+GString(id)+", cols: "+cols);
    }
    if( cols.length() )
    {
        pIdx = new IDX_INFO;
        cols = cols.stripTrailing(", ");
        pIdx->Iidx = id;
        pIdx->Name = name;
        pIdx->Columns = cols;
        pIdx->Type = unq;
        pIdx->CreateTime = crt;
        pIdx->StatsTime = mod;
        pIdx->IsDisabled = dis;
        indexSeq.add(pIdx);
        deb("::getIndexeInfo, final: "+GString(id)+", cols: "+cols);
    }

    cmd =
            "SELECT OBJECT_NAME(f.parent_object_id) t1, "               //TableName,
            " COL_NAME(fc.parent_object_id,fc.parent_column_id) t2, "   //ColName
            " OBJECT_NAME(f.referenced_object_id) t3, "                 //referenced table
            " OBJECT_NAME(f.object_id) t4, "                            //FKey name
            " COL_NAME(fc.referenced_object_id,fc.referenced_column_id) t5," //referenced col
            " (f.delete_referential_action) t6,"                          //on delete
            " (f.delete_referential_action_desc) t7,"                    //"CASCADE"
            " (f.update_referential_action) t8,"
            " (f.update_referential_action_desc) t9,	"
            " f.create_date t10, f.modify_date t11 "

            " FROM sys.foreign_keys AS f INNER JOIN sys.foreign_key_columns AS fc "
            " ON f.OBJECT_ID = fc.constraint_object_id INNER JOIN "
            " sys.tables t  ON t.OBJECT_ID = fc.referenced_object_id "
            " WHERE    OBJECT_NAME (f.parent_object_id) = '"+tabName(table)+"'";

    GString keyName, refCols;
    cols = "";
    deb("::getIndexeInfo, cmd: "+cmd);
    this->initAll(cmd);
    for( unsigned long i = 1; i <= this->numberOfRows(); ++i )
    {
        pIdx = new IDX_INFO;
        if( keyName == this->rowElement(i, 4).strip("'") || keyName == "")
        {
            cols += "\""+this->rowElement(i, 2).strip("'") +"\", ";
            refCols += "\""+this->rowElement(i, 5).strip("'") +"\", ";
            keyName = this->rowElement(i, 4).strip("'");
        }
        if( keyName != this->rowElement(i, 4).strip("'") || i == this->numberOfRows() )
        {
            pIdx->Iidx = "-1";
            pIdx->Name = keyName;
            //rowData->Columns = "["+cols.stripTrailing(", ")+"] => "+this->rowElement(i, 3)+"["+refCols.stripTrailing(", ")+"]";
            pIdx->Columns = cols.strip().stripTrailing(",");
            pIdx->Type = DEF_IDX_FORKEY;
            pIdx->RefTab = this->rowElement(i, 3);
            pIdx->RefCols = refCols.strip().stripTrailing(",");
            pIdx->CreateTime = this->rowElement(i, "t10").strip("'");
            pIdx->StatsTime = this->rowElement(i, "t11").strip("'");

            if( this->rowElement(1, "t6").strip("'").asInt() == 1 ) pIdx->DeleteRule = "ON DELETE "+this->rowElement(1, "t7").strip("'");
            if( this->rowElement(1, "t8").strip("'").asInt() == 1 ) pIdx->DeleteRule = "ON UPDATE "+this->rowElement(1, "t9").strip("'");
            keyName = "";
            indexSeq.add(pIdx);
        }
    }


    for( int i = 1; i <= (int) indexSeq.numberOfElements(); ++i )
    {
        pIdx = indexSeq.elementAtPosition(i);
        if( pIdx->Type == DEF_IDX_DUPL )pIdx->Stmt = getIndexStatement(0, table, pIdx->Name);
        if( pIdx->Type == DEF_IDX_FORKEY )pIdx->Stmt = getForeignKeyStatement(table, pIdx->Name);
        if( pIdx->Type == DEF_IDX_UNQ )pIdx->Stmt = getIndexStatement(0, table, pIdx->Name);
        if( pIdx->Type == DEF_IDX_PRIM )pIdx->Stmt = getIndexStatement(1, table, pIdx->Name);
    }

    return indexSeq;
}


GString pgsqlCLI::getChecks(GString, GString )
{
    return "";
}

GSeq <GString> pgsqlCLI::getChecksSeq(GString, GString)
{
    GSeq <GString> aSeq;
    return aSeq;
}



GString pgsqlCLI::getIndexStatement(int type, GString table, GString indexName)
{
    GString indCmd, unq, incStatement, order;
    int indexID = -1;
    indexName = indexName.strip("'");
    GString cmd = "select i.name, c.name, i.index_id, i.is_primary_key, i.is_unique,"
                  "i.is_disabled , i.is_hypothetical, ic.key_ordinal, ic.is_included_column, ic.is_descending_key "
                  "from sys.tables t  inner join sys.schemas s on t.schema_id = s.schema_id "
                  "inner join sys.indexes i on i.object_id = t.object_id "
                  "inner join sys.index_columns ic on ic.object_id = t.object_id and i.index_id = ic.index_id "
                  "inner join sys.columns c on c.object_id = t.object_id and ic.column_id = c.column_id "
                  "where s.name='"+tabSchema(table)+"' and t.name='"+tabName(table)+"' "
                                                                                    "and i.is_primary_key = '"+GString(type)+"' and i.name='"+indexName+"' order by i.index_id";

    deb("::getIndexStatement, cmd: "+cmd);

    this->initAll(cmd);
    deb("::getIndexes, found: "+GString(this->numberOfRows()));
    if( incStatement.length() ) incStatement= " INCLUDE ( "+incStatement.stripTrailing(",")+")";
    if( indexID >= 0 ) indCmd = indCmd.stripTrailing(",")+")"+incStatement+";";

    incStatement = "";
    indCmd = "";
    if( this->rowElement(1, 5).strip("'").asInt() == 1 ) unq = " UNIQUE ";
    else unq = "";

    indexID = this->rowElement(1, 3).strip("'").asInt();
    deb("::getIndexStatement, newInd: "+GString(indexID));

    //Get PrimaryKeys only
    if( type == 1 && this->rowElement(1, 4).strip("'").asInt() == 1 )
    {
        indCmd += "ALTER TABLE "+table+" ADD CONSTRAINT "+this->rowElement(1, 1)+" PRIMARY KEY(";
    }
    else if(type == 0 && this->rowElement(1, 4).strip("'").asInt() == 0) //Ordinary index
    {
        indCmd += "CREATE "+unq+" INDEX "+this->rowElement(1, 1)+" ON "+table + "(";
    }
    if( this->rowElement(1, 10).asInt() == 1 ) order = " DESC";
    else order = " ASC";
    if(this->rowElement(1, 9).asInt() == 1 ) incStatement +=  this->rowElement(1, 2).strip("'")+",";
    else indCmd += this->rowElement(1, 2).strip("'")+order+",";
    deb("::getIndexStatement, indCmd(1): "+indCmd+", inc: "+incStatement+", col9: "+this->rowElement(1, 9));

    if( incStatement.length() ) incStatement= " INCLUDE ( "+incStatement.stripTrailing(",")+")";
    if( indexID >= 0 ) indCmd = indCmd.stripTrailing(",")+")"+incStatement;


    deb("::getIndexStatement, indCmd(Fin): "+indCmd);

    return indCmd + ";";

}

GString pgsqlCLI::getForeignKeyStatement(GString table, GString foreignKeyName )
{
    setDatabaseContext(context(table));
    foreignKeyName = foreignKeyName.strip("'");

    GString out;
    GString cmd =
            "SELECT OBJECT_NAME(f.parent_object_id) t1, "               //TableName,
            " COL_NAME(fc.parent_object_id,fc.parent_column_id) t2, "   //ColName
            " OBJECT_NAME(f.referenced_object_id) t3, "                 //referenced table
            " OBJECT_NAME(f.object_id) t4, "                            //FKey name
            " COL_NAME(fc.referenced_object_id,fc.referenced_column_id) t5," //referenced col
            " (f.delete_referential_action) t6,"                          //on delete
            " (f.delete_referential_action_desc) t7,"                    //"CASCADE"
            " (f.update_referential_action) t8,"
            " (f.update_referential_action_desc) t9	"
            " FROM sys.foreign_keys AS f INNER JOIN sys.foreign_key_columns AS fc "
            " ON f.OBJECT_ID = fc.constraint_object_id INNER JOIN "
            " sys.tables t  ON t.OBJECT_ID = fc.referenced_object_id "
            " WHERE    OBJECT_NAME (f.parent_object_id) = '"+tabName(table)+"' AND OBJECT_NAME(f.object_id)='"+foreignKeyName+"'";

    GString keyName, cols, refCols;
    deb("::getForeignKeys, cmd: "+cmd);
    this->initAll(cmd);
    out = "";
    if( keyName == this->rowElement(1, 4).strip("'") || keyName == "")
    {
        cols += this->rowElement(1, 2).strip("'") +", ";
        refCols += this->rowElement(1, 5).strip("'") +", ";
        keyName = this->rowElement(1, 4).strip("'");
    }
    if( keyName != this->rowElement(1, 4).strip("'") || 1 == this->numberOfRows() )
    {
        out += "ALTER TABLE "+table+" ADD CONSTRAINT "+this->rowElement(1, 4).strip("'")+" FOREIGN KEY ("+cols.stripTrailing(", ")+") REFERENCES "+
                this->rowElement(1, 3).strip("'")+"("+refCols.stripTrailing(", ")+")";
        if( this->rowElement(1, "t6").strip("'").asInt() == 1 ) out += " ON DELETE "+this->rowElement(1, "t7").strip("'");
        if( this->rowElement(1, "t8").strip("'").asInt() == 1 ) out += " ON UPDATE "+this->rowElement(1, "t9").strip("'");
        keyName = "";
    }
    return out.removeButOne() + ";";
}





int pgsqlCLI::hasUniqueConstraint(GString tableName)
{

    GSeq <GString>  colSeq;
    if( this->getUniqueCols(tableName, &colSeq)) return 0;
    return colSeq.numberOfElements();

    /////////////////////////////////////////////////////////////////////
    // DEAD CODE AHEAD
    /////////////////////////////////////////////////////////////////////
    GString cmd = "select count(*) from INFORMATION_SCHEMA.CONSTRAINT_COLUMN_USAGE ";
    cmd += "where table_catalog='"+context(tableName)+"' and TABLE_SCHEMA='"+tabSchema(tableName)+"' and ";
    cmd += "TABLE_NAME='"+tabName(tableName)+"'";

    this->initAll(cmd);
    deb("hasUniqueConstraint, first check: "+this->rowElement(1,1));
    if(this->rowElement(1,1).asInt() > 0 ) return 1;

    cmd = "SELECT count(*) FROM sys.[tables] AS BaseT  INNER JOIN sys.[indexes] I ON BaseT.[object_id] = I.[object_id]  ";
    cmd += "INNER JOIN sys.[index_columns] IC ON I.[object_id] = IC.[object_id] ";
    cmd += "INNER JOIN sys.[all_columns] AC ON BaseT.[object_id] = AC.[object_id] AND IC.[column_id] = AC.[column_id] ";
    cmd += "WHERE BaseT.[is_ms_shipped] = 0 AND I.[type_desc] <> 'HEAP' ";
    cmd += "and OBJECT_SCHEMA_NAME(BaseT.[object_id],DB_ID()) = '"+tabSchema(tableName)+"' and ";
    cmd += "BaseT.[name] ='"+tabName(tableName)+"' and I.[is_unique] >0";

    this->initAll(cmd);
    deb("hasUniqueConstraint, second check: "+this->rowElement(1,1));
    if(this->rowElement(1,1).asInt() > 0 ) return 1;

    return 0;
}


int pgsqlCLI::getUniqueCols(GString table, GSeq <GString> * colSeq)
{
    setDatabaseContext(context(table));
    int indexID = -1;
    GString cmd = "select c.name, i.index_id "
                  "from sys.tables t  inner join sys.schemas s on t.schema_id = s.schema_id "
                  "inner join sys.indexes i on i.object_id = t.object_id "
                  "inner join sys.index_columns ic on ic.object_id = t.object_id and i.index_id = ic.index_id "
                  "inner join sys.columns c on c.object_id = t.object_id and ic.column_id = c.column_id "
                  "where s.name='"+tabSchema(table)+"' and t.name='"+tabName(table)+"' "
                                                                                    "and i.is_unique >0 "
                                                                                    "order by i.is_primary_key desc, index_id" ;
    deb("getUniqueCols, cmd: "+cmd);
    this->initAll(cmd);
    deb("getUniqueCols, cols: "+GString(this->numberOfRows()));
    if( this->numberOfRows() == 0 ) return 1;

    for(unsigned long  i=1; i<=this->numberOfRows(); ++i )
    {
        if( this->rowElement(i, 3).strip("'").asInt() != indexID )
        {
            if( indexID >= 0 && this->rowElement(i, 2).strip("'").asInt() != indexID) break;
            indexID = this->rowElement(i, 2).strip("'").asInt();
            colSeq->add(this->rowElement(i, 1).strip("'"));
        }
    }
    return 0;
}

GString pgsqlCLI::tabName(GString table, int removeDoubleQuotes)
{
    table = table.removeAll('\"');
    deb("pgsqlCLI::tabName: "+table);
    if( table.occurrencesOf(".") != 2 && table.occurrencesOf(".") != 1) return "@ErrTabString";
    if( table.occurrencesOf(".") == 2 ) table = table.remove(1, table.indexOf("."));
    return table.subString(table.indexOf(".")+1, table.length()).strip();
}
GString pgsqlCLI::context(GString table)
{
    table.removeAll('\"');
    deb("pgsqlCLI::context for "+table);
    if( table.occurrencesOf(".") != 2 ) return "";
    return table.subString(1, table.indexOf(".")-1);
}

GString pgsqlCLI::tabSchema(GString table, int removeDoubleQuotes)
{
    if( removeDoubleQuotes) table.removeAll('\"');
    deb("pgsqlCLI::tabSchema: "+table);
    if( table.occurrencesOf(".") != 2 && table.occurrencesOf(".") != 1) return "@ErrTabString";
    if( table.occurrencesOf(".") == 2 ) table.remove(1, table.indexOf("."));
    return table.subString(1, table.indexOf(".")-1);
}

void pgsqlCLI::createXMLCastString(GString &xmlData)
{
    deb("::createXMLCastString called");
    xmlData = " cast('"+(xmlData)+"' as xml)";
}

int pgsqlCLI::isBinary(unsigned long row, int col)
{
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return 0;
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    if( row < 1 || (unsigned long) col > aRow->elements() ) return 0;
    return aRow->rowElement(col)->isBinary;
}

int  pgsqlCLI::isNull(unsigned long row, int col)
{
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return 0;
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    if( col < 1 || (unsigned long) col > aRow->elements() ) return 0;
    deb("::isNull: "+GString(aRow->rowElement(col)->isNull)+", row: "+GString(row)+", col: "+GString(col));
    return aRow->rowElement(col)->isNull;
}

int pgsqlCLI::isTruncated(unsigned long row, int col)
{
    if( row < 1 || row > allRowsSeq.numberOfElements() ) return 0;
    GRowHdl *aRow;
    aRow = allRowsSeq.elementAtPosition(row);
    if( row < 1 || (unsigned long) col > aRow->elements() ) return 0;
    return aRow->rowElement(col)->isTruncated;
}
void pgsqlCLI::deb(GString fnName, GString txt)
{
    if( m_pGDB ) m_pGDB->debugMsg("pgsqlCLI", m_iMyInstance, "::"+fnName+" "+txt);
}

void pgsqlCLI::deb(GString text)
{
    if( m_pGDB ) m_pGDB->debugMsg("pgsqlCLI", m_iMyInstance, text);
}
void pgsqlCLI::setTruncationLimit(int limit)
{
    //Setting m_iTruncationLimit to 0 -> no truncation
    m_iTruncationLimit = limit;
}
int pgsqlCLI::uploadLongBuffer( GString cmd, GString data, int isBinary )
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(data);
    PMF_UNUSED(isBinary);
    return -1;
}
GString pgsqlCLI::cleanString(GString in)
{
    if( in.length() < 2 ) return in;
    if( in[1UL] == '\'' && in[in.length()] == '\'' ) return in.subString(2, in.length()-2);
    return in;
}
int pgsqlCLI::isLongTypeCol(int i)
{
    if( i < 1 || (unsigned long)i > sqlLongTypeSeq.numberOfElements() ) return 0;
    return sqlLongTypeSeq.elementAtPosition(i);
}
void pgsqlCLI::getResultAsHEX(int asHex)
{
    PMF_UNUSED(asHex);
    //TODO
}
void pgsqlCLI::setReadUncommitted(short readUncommitted)
{
    m_iReadUncommitted = readUncommitted;
    if( m_iReadUncommitted ) readRowData("set transaction isolation level read uncommitted");
    else readRowData("set transaction isolation level REPEATABLE READ");
}

void pgsqlCLI::setGDebug(GDebug *pGDB)
{
    m_pGDB = pGDB;
}
void pgsqlCLI::setDatabaseContext(GString context)
{
    deb("setting context: "+context);
    m_strContext = context;
    //this->initAll("use \""+context+"\"");
    this->initAll("use "+context);
}

int pgsqlCLI::exportAsTxt(int mode, GString sqlCmd, GString table, GString outFile, GSeq <GString>* startText, GSeq <GString>* endText, GString *err)
{
    PMF_UNUSED(mode);
    deb("exportAsTxt start. cmd: "+sqlCmd+", table: "+table+", out: "+outFile);
    GFile f(outFile, GF_APPENDCREATE);
    if( !f.initOK() )
    {
        *err = "Could not open file "+outFile+" for writing.";
        return 1;
    }
    GSeq <GString> aSeq;
    aSeq.add("--------------------------------------- ");
    aSeq.add("-- File created by Poor Man's Flight (PMF) www.leipelt.de");
    aSeq.add("-- Statement: "+sqlCmd);
    aSeq.add("-- ");
    aSeq.add("-- NOTE: LOBs will be given as NULL. ");
    aSeq.add("--------------------------------------- ");
    aSeq.add("");
    aSeq.add("-- Use the following line to set the target database:");
    aSeq.add("-- USE [DataBase] ");
    aSeq.add("");

    aSeq.add("-- SET IDENTITY_INSERT "+table+" ON");

    for(unsigned long i = 1; i <= startText->numberOfElements(); ++i) aSeq.add(startText->elementAtPosition(i));

    *err = this->readRowData(sqlCmd, 0, 0, 1);
    if(err->length()) return this->sqlCode();

    remove(outFile);


    GString columns = "INSERT INTO "+table+"(";
    for(unsigned long i = 1; i <= this->numberOfColumns(); ++i)
    {
        columns += "["+this->hostVariable(i)+"], ";
    }
    columns = columns.stripTrailing(", ")+") VALUES (";

    deb("exportAsTxt cols is: "+columns);

    GString out, data;
    for(unsigned long i = 1; i <= this->numberOfRows(); ++i)
    {
        out = "";
        for(unsigned long j = 1; j <= this->numberOfColumns(); ++j)
        {
            data = this->rowElement(i, j);
            if( this->isNumType(j))  out += cleanString(data)+", ";
            else if( data == "NULL" )out += data+", ";
            else if( this->sqlType(j) == SQLSRV_NVARCHAR )out += "N"+data+", ";
            else if( data.occurrencesOf("@DSQL@") ) out +="NULL, ";
            else out += data+", ";
        }
        out = out.stripTrailing(", ") + ")";
        aSeq.add(columns+out);
        deb("exportAsTxt adding  "+out);
        if( aSeq.numberOfElements() > 1000)
        {
            f.append(&aSeq);
            aSeq.removeAll();
            deb("exportAsTxt appending to "+outFile);
        }
    }
    for(unsigned long i = 1; i <= endText->numberOfElements(); ++i) aSeq.add(endText->elementAtPosition(i));
    aSeq.add("-- SET IDENTITY_INSERT "+table+" OFF");
    f.append(&aSeq);

    return 0;
}
int pgsqlCLI::deleteTable(GString tableName)
{
    this->initAll("drop table "+tableName);
    if( sqlCode() && this->getDdlForView(tableName).length())
    {
        this->initAll("drop view "+tabSchema(tableName)+"."+tabName(tableName));
    }
    return sqlCode();
}
//GString pgsqlCLI::fillIndexViewRawData(GString table)
//{

//    this->initAll("USE \""+context(table)+"\"");

//    GString cmd = "SELECT * FROM sys.[tables] AS BaseT  "
//			"INNER JOIN sys.[indexes] I ON BaseT.[object_id] = I.[object_id]  "
//			"INNER JOIN sys.[index_columns] IC ON I.[object_id] = IC.[object_id] "
//			"INNER JOIN sys.[all_columns] AC ON BaseT.[object_id] = AC.[object_id] AND IC.[column_id] = AC.[column_id] "
//			"WHERE BaseT.[is_ms_shipped] = 0 AND I.[type_desc] <> 'HEAP' "
//			"and OBJECT_SCHEMA_NAME(BaseT.[object_id],DB_ID()) = '"+tabSchema(table)+"'"
//            "and  BaseT.[name] ='"+tabName(table)+"'"+
//			"ORDER BY BaseT.[name], I.[index_id], IC.[key_ordinal] ";
//    this->initAll(cmd);
//	deb("::fillIndexView, cmd: "+cmd);
//    deb("::fillIndexView, found: "+GString(this->numberOfRows()));

//	clearSequences();
//    for(unsigned long i = 1; i <= this->numberOfColumns(); ++i )
//	{
//		headerSeq.add(this->hostVariable(i));
//	}

//	RowData * rowData;
//    for(unsigned long  i=1; i<=this->numberOfRows(); ++i )
//    {
//		rowData = new RowData;
//		deb("::fillIndexView, i: "+GString(i));
//        for(unsigned long j = 1; j <= this->numberOfColumns(); ++j )
//		{
//			rowData->add(this->rowElement(i, j));
//		}
//		rowSeq.add(rowData);
//	}
//	return "";
//}


GString pgsqlCLI::fillChecksView(GString, int)
{
    return "";
}

pgsqlCLI::RowData::~RowData()
{
    rowDataSeq.removeAll();
}
void pgsqlCLI::RowData::add(GString data)
{
    rowDataSeq.add(data);
}
GString pgsqlCLI::RowData::elementAtPosition(int pos)
{
    if( pos < 1 || pos > (int)rowDataSeq.numberOfElements() ) return "";
    return rowDataSeq.elementAtPosition(pos);
}
int pgsqlCLI::getHeaderData(int pos, GString * data)
{
    if( pos < 1 || pos > (int)headerSeq.numberOfElements()) return 1;
    *data = headerSeq.elementAtPosition(pos);
    return 0;
}

int pgsqlCLI::getRowData(int row, int col, GString * data)
{
    if( row < 1 || row > (int)rowSeq.numberOfElements() ) return 1;
    if( col < 1 || col > (int)rowSeq.elementAtPosition(row)->numberOfElements() ) return 1;
    *data = rowSeq.elementAtPosition(row)->elementAtPosition(col);
    return 0;
}

unsigned long pgsqlCLI::RowData::numberOfElements()
{
    return rowDataSeq.numberOfElements();
}
unsigned long pgsqlCLI::getHeaderDataCount()
{
    return headerSeq.numberOfElements();
}
unsigned long pgsqlCLI::getRowDataCount()
{
    return rowSeq.numberOfElements();
}

int pgsqlCLI::isTransaction()
{
    return m_iIsTransaction;
}

void pgsqlCLI::clearSequences()
{
    RowData *pRowData;
    while( !rowSeq.isEmpty() )
    {
        pRowData = rowSeq.firstElement();
        delete pRowData;
        rowSeq.removeFirst();
    }
    headerSeq.removeAll();
}

GString pgsqlCLI::getDdlForView(GString table)
{

    table = tabName(table);
    this->initAll("SELECT v.VIEW_DEFINITION FROM INFORMATION_SCHEMA.VIEWS v WHERE TABLE_NAME='"+table+"'");
    if( this->numberOfRows() == 0 ) return "";
    return cleanString(this->rowElement(1, 1).strip());
    //this->initAll("select OBJECT_ID ('"+table+"', 'V') ");
    //return this->rowElement(1, 1) == "NULL" ? "" : "ObjectId: "+this->rowElement(1, 1);
}

void pgsqlCLI::setAutoCommmit(int commit)
{
    SQLSetConnectAttr(m_SQLHDBC, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)true, commit);
}

int pgsqlCLI::execByDescriptor( GString cmd, GSeq <GString> *dataSeq)
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(dataSeq);
    return -1;
}
int pgsqlCLI::execByDescriptor( GString cmd, GSeq <GString> *dataSeq, GSeq <int> *typeSeq,
                                GSeq <short>* sqlVarLengthSeq, GSeq <int> *forBitSeq )
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(dataSeq);
    PMF_UNUSED(typeSeq);
    PMF_UNUSED(sqlVarLengthSeq);
    PMF_UNUSED(forBitSeq);
    return -1;
}
long pgsqlCLI::uploadBlob(GString cmd, char * buffer, long size)
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(buffer);
    PMF_UNUSED(size);
    return -1;
}

long pgsqlCLI::retrieveBlob( GString cmd, GString &blobFile, int writeFile )
{
    PMF_UNUSED(cmd);
    PMF_UNUSED(blobFile);
    PMF_UNUSED(writeFile);
    return -1;
}

void pgsqlCLI::currentConnectionValues(CON_SET * conSet)
{
    conSet->DB = m_strDB;
    conSet->Host = m_strHost;
    conSet->PWD = m_strPWD;
    conSet->UID = m_strUID;
    conSet->Port = m_strPort;
    conSet->CltEnc = m_strCltEnc;
    conSet->Type = _PGSQLCLI;
}

GString pgsqlCLI::lastSqlSelectCommand()
{
    return m_strLastSqlSelectCommand;;
}

TABLE_PROPS pgsqlCLI::getTableProps(GString )
{
    TABLE_PROPS tableProps;
    tableProps.init();
    return tableProps;
}

int pgsqlCLI::tableIsEmpty(GString tableName)
{
    GString err = this->initAll("select top 1 * from " + tableName);
    if( this->numberOfRows() == 0 || err.length() ) return 1;
    return 0;
}

int pgsqlCLI::deleteViaFetch(GString tableName, GSeq<GString> * colSeq, int rowCount, GString whereClause)
{
    GString cmd = "DELETE TOP ("+GString(rowCount)+") FROM "+tableName;
    if( whereClause.length() ) cmd += " WHERE "+whereClause;
    GString err = this->initAll(cmd);
    deb("deleteViaFetch: cmd: "+cmd+"\nErr: "+err);
    if( err.length() ) return sqlCode();
    this->commit();
    return 0;
}

GString pgsqlCLI::setEncoding(GString encoding)
{
    m_strCltEnc = encoding;
    return "";
}

void pgsqlCLI::getAvailableEncodings(GSeq<GString> *encSeq)
{
    PMF_UNUSED(encSeq);
}

/******************************************************************
*
*  ifdef: Use QT for some tasks
*
*****************************************************************/
#ifdef  QT4_DSQL
GString pgsqlCLI::getIdenticals(GString table, QWidget* parent, QListWidget *pLB, short autoDel)
{

    GString message = "SELECT * FROM "+table;
    GString retString = "";
    deb("::getIdenticals, cmd: "+message);


    if( m_SQLHDBC == SQL_NULL_HDBC )
    {
        deb("::getIdenticals: No handle, quitting.");
        return "";
    }

    //Clear all internal sequences:
    resetAllSeq();

    SQLRETURN ret = 0;
    SQLSMALLINT columns;

    SQLFreeHandle(SQL_HANDLE_DBC, m_SQLHDBC);
    ret = SQLAllocHandle(SQL_HANDLE_STMT, m_SQLHDBC, &m_SQLHSTMT);
    deb("::getIdenticals, SQLAllocHandle, rc: "+GString(ret));
    if(ret) return sqlError();

    ret = SQLExecDirect(m_SQLHSTMT, (SQLCHAR*)message, SQL_NTS);
    deb("::getIdenticals, SQLExecDirect, rc: "+GString(ret));
    if(ret) return sqlError();


    ret = SQLNumResultCols(m_SQLHSTMT, &columns);

    deb("::getIdenticals, SQLNumResultCols, rc: "+GString(ret)+", cols: "+GString(columns));
    if(ret) return sqlError();
    m_iNumberOfColumns = columns;

    getColInfo(&m_SQLHSTMT);

    char *buf;
    GString data, countCmd, out;

    pgsqlCLI * tmpODBC = new pgsqlCLI(*this);
    pgsqlCLI * delODBC = new pgsqlCLI(*this);

    int blockSize = 2000;
    unsigned long lns =0, perc = 0, pass = 1;
    QProgressDialog * apd = NULL;
    apd = new QProgressDialog(GString("Searching in "+table), "Cancel", 0, blockSize, parent);
    apd->setWindowModality(Qt::WindowModal);
    apd->setValue(1);

    while (SQL_SUCCEEDED(ret = SQLFetch(m_SQLHSTMT)))
    {
        SQLLEN len = 0;
        int bufLen;
        if( !(perc % 10) ) apd->setValue(perc);
        if( apd->wasCanceled() )
        {
            retString = "QUIT";
            break;
        }

        countCmd = "";
        for ( int i = 1; i <= m_iNumberOfColumns; i++ )
        {
            if(isXMLCol(i) || isLOBCol(i))continue;
            bufLen = sqlVarLengthSeq.elementAtPosition(i)+1;

            buf = new char[bufLen];
            ret = SQLGetData(m_SQLHSTMT, i, SQL_C_CHAR, buf, bufLen, &len);

            //deb("::initAll, row: "+GString(m_iNumberOfRows)+", col: "+GString(i)+", ret: "+GString(ret)+", len: "+GString(len)+", bufLen: "+GString(bufLen)+", blob: "+GString(lobSeq.elementAtPosition(i))+", xml: "+GString(xmlSeq.elementAtPosition(i)));
            deb("::getIdenticals, row: "+GString(m_iNumberOfRows)+", col: "+GString(i)+", ret: "+GString(ret)+", len: "+GString((int)len)+", bufLen: "+GString(bufLen));
            if (len == SQL_NULL_DATA)  data = " IS NULL";
            else
            {
                data = buf;
                deb("Buf is valid, data: "+data);
                if( sqlTypeSeq.elementAtPosition(i) == SQLSRV_DATETIME ) handleDateTimeString(data);
                if( !isNumType(i)) data = " = '"+data+"'";
                else data = " = "+data;
            }
            delete[] buf;
            countCmd += hostVariable(i)+data+ " AND ";
        }
        countCmd = countCmd .stripTrailing(" AND ");
        lns++;
        perc++;
        if( (int)perc > blockSize )
        {
            pass++;
            apd->setLabelText(table+", chunk #"+GString(pass)+" (2000 rows per chunk)");
            apd->setValue(pass);
            if( apd->wasCanceled() ) break;
            perc = 1;
        }
        deb("getIdenticals: CountCmd: "+countCmd);
        tmpODBC->initAll( "SELECT COUNT(*) FROM "+table+" WHERE "+countCmd);
        if(tmpODBC->rowElement(1,1).asInt() > 1)
        {
            out = table +": Found "+GString(tmpODBC->rowElement(1,1))+" results for "+countCmd;
            writeToLB(pLB, out);
        }
        for(int i=1; i<tmpODBC->rowElement(1,1).asInt() && autoDel; ++i)
        {
            delODBC->currentCursor("SELECT * FROM "+table+" WHERE "+countCmd, "DELETE FROM "+table, 1, 0);
        }

    }
    writeToLB(pLB, "-- Checked: "+table);
    if( apd )
    {
        apd->setValue(blockSize);
        delete apd;
    }

    delete tmpODBC;
    delete delODBC;
    deb("::getIdenticals, err fetch: "+GString(ret));
    freeStmt(m_SQLHSTMT);
    if( ret && ret != 100) return m_strLastError;
    return retString;
}

void pgsqlCLI::writeToLB(QListWidget * pLB, GString message)
{
    for( int i = 0; i < pLB->count(); ++i )
        if( GString(pLB->item(i)->text()) == message ) return;
    new QListWidgetItem(message, pLB);
}


GString  pgsqlCLI::fillIndexView(GString table, QWidget* parent, QTableWidget *pWdgt)
{
    PMF_UNUSED(parent);
    //Clear data:
    while( pWdgt->rowCount() ) pWdgt->removeRow(0);

    GString id, name, cols, unq, crt, mod, dis;
    int indexID = -1;
    setDatabaseContext(context(table));


    GString cmd = "select i.index_id, i.name, c.name,  i.is_primary_key, i.is_unique_constraint,"
                  "create_date, modify_date, i.is_disabled "
                  "from sys.tables t  inner join sys.schemas s on t.schema_id = s.schema_id "
                  "inner join sys.indexes i on i.object_id = t.object_id "
                  "inner join sys.index_columns ic on ic.object_id = t.object_id "
                  "inner join sys.columns c on c.object_id = t.object_id and ic.column_id = c.column_id "
                  "where s.name='"+tabSchema(table)+"' and t.name='"+tabName(table)+"'";
    this->initAll(cmd);
    deb("::fillIndexView, cmd: "+cmd);
    deb("::fillIndexView, found: "+GString(this->numberOfRows()));

    pWdgt->setColumnCount(7);

    QTableWidgetItem * pItem;
    int i = 0;
    pItem = new QTableWidgetItem("IndexID"); pWdgt->setHorizontalHeaderItem(i++, pItem);
    pItem = new QTableWidgetItem("Name"); pWdgt->setHorizontalHeaderItem(i++, pItem);
    pItem = new QTableWidgetItem("Columns"); pWdgt->setHorizontalHeaderItem(i++, pItem);
    pItem = new QTableWidgetItem("Type"); pWdgt->setHorizontalHeaderItem(i++, pItem);
    pItem = new QTableWidgetItem("Created"); pWdgt->setHorizontalHeaderItem(i++, pItem);
    pItem = new QTableWidgetItem("Modified"); pWdgt->setHorizontalHeaderItem(i++, pItem);
    pItem = new QTableWidgetItem("IsDisabled"); pWdgt->setHorizontalHeaderItem(i++, pItem);

    int row = 0;
    for(int  i=1; i<= (int)this->numberOfRows(); ++i )
    {
        deb("::fillIndexView, i: "+GString(i));
        if( this->rowElement(i, 1).strip("'").asInt() != indexID )
        {
            deb("::fillIndexView, i: "+GString(i)+", indexChg, new: "+GString(indexID));
            if( cols.length() )
            {
                cols = cols.stripTrailing(", ");
                createIndexRow(pWdgt, row++, id, name, cols, unq, crt, mod, dis);
            }
            id   = this->rowElement(i, 1);
            name = this->rowElement(i, 2);
            cols = "";
            if( this->rowElement(i, 4) == "'1'" ) unq = "Primary Key";
            else if( this->rowElement(i, 5) == "'1'" ) unq = "Unique Key";
            else unq = "";
            crt = this->rowElement(i, 6);
            mod = this->rowElement(i, 7);
            dis = this->rowElement(i, 8);
            indexID = this->rowElement(i, 1).strip("'").asInt();
            deb("::fillIndexView, i: "+GString(i)+", currIDx: "+GString(id)+", cols: "+cols);
        }
        cols += this->rowElement(i, 3).strip("'")+", ";
        deb("::fillIndexView, i: "+GString(i)+", currIDx: "+GString(id)+", cols: "+cols);
    }
    if( cols.length() )
    {
        cols = cols.stripTrailing(", ");
        createIndexRow(pWdgt, row++, id, name, cols, unq, crt, mod, dis);
        deb("::fillIndexView, final: "+GString(id)+", cols: "+cols);
    }
    return "";
}
void pgsqlCLI::createIndexRow(QTableWidget *pWdgt, int row,
                              GString id, GString name, GString cols, GString unq, GString crt, GString mod, GString dis)
{
    if( !id.length() ) return;
    deb("::fillIndexView, createRow, id: "+id);
    int j = 0;
    QTableWidgetItem * pItem;
    pWdgt->insertRow(pWdgt->rowCount());
    pItem = new QTableWidgetItem(); pItem->setText(id);   pWdgt->setItem(row, j++, pItem);
    pItem = new QTableWidgetItem(); pItem->setText(name); pWdgt->setItem(row, j++, pItem);
    pItem = new QTableWidgetItem(); pItem->setText(cols); pWdgt->setItem(row, j++, pItem);
    pItem = new QTableWidgetItem(); pItem->setText(unq);  pWdgt->setItem(row, j++, pItem);
    pItem = new QTableWidgetItem(); pItem->setText(crt);  pWdgt->setItem(row, j++, pItem);
    pItem = new QTableWidgetItem(); pItem->setText(mod);  pWdgt->setItem(row, j++, pItem);
    pItem = new QTableWidgetItem(); pItem->setText(dis);  pWdgt->setItem(row, j++, pItem);

}
#endif
/******************************************************************
*
*  END ifdef: Use QT for some tasks
*
*****************************************************************/

/*
QString pgsqlCLI::uuidToString(const QVariant &v)
{

// get pointer to raw data
QByteArray arr(v.toByteArray());
std::string result(arr.constData(),arr.size());
//assert(result.size() == 16);
const char *ptr = result.data();
// extract the GUID-parts from the data
uint   data1 = *reinterpret_cast<const uint*>(ptr);
ushort data2 = *reinterpret_cast<const ushort*>(ptr+=sizeof(uint));
ushort data3 = *reinterpret_cast<const ushort*>(ptr+=sizeof(ushort));
uchar  data4[8] =
{
    *reinterpret_cast<const uchar*>(ptr+=sizeof(ushort)),
    *reinterpret_cast<const uchar*>(++ptr),
    *reinterpret_cast<const uchar*>(++ptr),
    *reinterpret_cast<const uchar*>(++ptr),
    *reinterpret_cast<const uchar*>(++ptr),
    *reinterpret_cast<const uchar*>(++ptr),
    *reinterpret_cast<const uchar*>(++ptr),
    *reinterpret_cast<const uchar*>(++ptr)
};
// create a uuid from the extracted parts
QUuid uuid(
            data1,
            data2,
            data3,
            data4[0],
        data4[1],
        data4[2],
        data4[3],
        data4[4],
        data4[5],
        data4[6],
        data4[7]);
// finally return the uuid as a QString
return uuid.toString();

}
*/




//UPDATE Person.Address SET StateProvinceID=1 WHERE AddressID = 1  AND rowguid = '9AADCB0D-36CF-483F-84D8-585C2D4EC6E9'
