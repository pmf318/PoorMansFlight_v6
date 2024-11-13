#ifndef HELPER_H
#define HELPER_H

#include <QLabel>
#include <gstring.hpp>
#include <idsql.hpp>
#include <dsqlplugin.hpp>

#include <QItemSelectionModel>
#include <QTableWidget>

#ifndef pmf_min
#define pmf_min(A,B) ((A) <(B) ? (A):(B))
#endif


class Helper
{
private:
    static int findNextSortToken(GString in);

public:
    static QModelIndexList getSelectedRows(QItemSelectionModel* selectionModel);
    static void msgBox(QWidget *parent, QString title, QString msg );
    static GString tempPath();
    static GString fileNameFromPath(GString fullPath);
    static GString filePath(GString path, GString file);
    static GString createIndexSortString(GString in);
    static bool removeDir(const QString & dirName);
    static void showHintMessage(QWidget * parent, int id);
    static int askFileOverwrite(QWidget *parent, GString fileName, GString question = "");
    static int fileIsUtf8(GString filename);
    static int convertBase64toPNG(GString fileName, GString * newExt);
    static int uncompressGZ(GString fileName);
    static QByteArray gUncompress(const QByteArray &data);
    static bool dataIsXML(QByteArray * array);
    static GString pmfVersion();
    static GString pmfNameAndVersion(GString database = "");
    static GString tableName(GString table, ODBCDB type);
    static GString tableSchema(GString table, ODBCDB type);
    static GString tableContext(GString table, ODBCDB type);
    static void setVHeader(QTableWidget * someLV, bool movable = true);

    static void setLastSelectedPath(GString className, GString path);
    static GString getLastSelectedPath(GString className);
    static GString formatForHex(DSQLPlugin* pDSQL, QTableWidgetItem * pItem);
    static GString formatForHex(DSQLPlugin* pDSQL, GString in);
    static GString formatItemText(DSQLPlugin * pDSQL, QTableWidgetItem * pItem);
    static int isSystemString(GString in);
    static GString createSearchConstraint(DSQLPlugin * pDSQL, GSeq<COL_SPEC*> *colDescSeq, GString val, int col, int exactMatch);
    static GString wrap(GString in);
    static int createFileIfNotExist(GString fileName);
    static int fileExists(GString fileName);
    static GString convertGuid(GString in);
    static void setGeometry(QDialog *qd, GString key);
    static void storeGeometry(QDialog *qd, GString key);
    static GString connSetToString(CON_SET * pCS);
    static int connSetFromString(GString txt, CON_SET * pCS);
    static GString handleGuid(DSQLPlugin *pDSQL,  GString in, int convertGuid);
    static int runStuffInProcess(GString cmd, GString &res, GString &err);
	static int runCommandInProcess(GString cmd, GString &res, GString &err);
	static bool isSystemDarkPalette();
    static GString getSensibleStyle();
    //static int convertFileToCodePage(GString inFile, GString outFile, GString codec);

};

#endif // HELPER_H
