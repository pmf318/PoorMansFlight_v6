

#include "sqlHighlighter.h"
#include "helper.h"
#include <gseq.hpp>
#include <QMessageBox>


/******************************************************************
 * QT 6
*******************************************************************/
#if QT_VERSION >= 0x060000
SqlHighlighter::SqlHighlighter( PmfColorScheme colorScheme, QTextDocument *parent, GSeq<GString> *sqlCmdList, GSeq<GString> *hostVarList)  : QSyntaxHighlighter(parent)
{

    m_iColorScheme = colorScheme;
    HighlightingRule sqlSyntaxRule;
    HighlightingRule colNamesRule;

    if(!Helper::isSystemDarkPalette()) 
	{
		sqlKeywordFormat.setForeground(Qt::darkBlue);
		colNameFormat.setForeground(Qt::darkGreen);
	}
	else
	{
		sqlKeywordFormat.setForeground(Qt::cyan);
		colNameFormat.setForeground(Qt::magenta);
	}


    QStringList sqlKeywordPatterns;
    QStringList colNamesPatterns;


    if( hostVarList != NULL)
    {
        for( int i = 1; i <= (int)hostVarList->numberOfElements(); ++i)
        {
            if( !hostVarList->elementAtPosition(i).strip().length() ) continue;
            colNamesPatterns << "\\b"+hostVarList->elementAtPosition(i).strip("\"")+"\\b";
        }
    }

    if( sqlCmdList != NULL )
    {
        for( int i = 1; i <= (int)sqlCmdList->numberOfElements(); ++i)
        {
            if( !sqlCmdList->elementAtPosition(i).strip().length() ) continue;
            sqlKeywordPatterns << "\\b"+sqlCmdList->elementAtPosition(i)+"\\b";
        }

    }

    foreach (const QString &pattern, sqlKeywordPatterns)
    {
        sqlSyntaxRule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        sqlSyntaxRule.format = sqlKeywordFormat;
        highlightingRules.append(sqlSyntaxRule);
    }

    foreach (const QString &pattern, colNamesPatterns)
    {
        colNamesRule.pattern = QRegularExpression(pattern,  QRegularExpression::CaseInsensitiveOption);
        colNamesRule.format = colNameFormat;
        highlightingRules.append(colNamesRule);
    }
    /*
    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkMagenta);
    sqlSyntaxRule.pattern = QRegExp("\\b[A-Za-z0-9_]+\\b");
    sqlSyntaxRule.format = classFormat;
    highlightingRules.append(sqlSyntaxRule);

    quotationFormat.setForeground(Qt::darkGreen);
    sqlSyntaxRule.pattern = QRegExp("\".*\"");
    sqlSyntaxRule.format = quotationFormat;
    highlightingRules.append(sqlSyntaxRule);


    functionFormat.setFontItalic(true);
    functionFormat.setForeground(Qt::blue);
    sqlSyntaxRule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
    sqlSyntaxRule.format = functionFormat;
    highlightingRules.append(sqlSyntaxRule);
    */
    if(!Helper::isSystemDarkPalette()) singleLineCommentFormat.setForeground(Qt::darkGray);
	singleLineCommentFormat.setForeground(Qt::gray);
    sqlSyntaxRule.pattern = QRegularExpression("--[^\n]*");
    sqlSyntaxRule.format = singleLineCommentFormat;
    highlightingRules.append(sqlSyntaxRule);

    if(!Helper::isSystemDarkPalette()) multiLineCommentFormat.setForeground(Qt::red);
	else multiLineCommentFormat.setForeground(Qt::darkGray);

    sqlValStartExpression= QRegularExpression("'");
    sqlValEndExpression= QRegularExpression("'");

    txtValStartExpression= QRegularExpression("\"");
    txtValEndExpression= QRegularExpression("\"");

    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}
void SqlHighlighter::deb(GString msg)
{
    return;
    printf("SqlHighlighter: %s\n", (char*) msg);
#if defined(MAKE_VC) || defined (__MINGW32__)
    _flushall();
#endif
}

void SqlHighlighter::highlightBlock(const QString &text)
{
    if( !Helper::isSystemDarkPalette() )
    {
        highlightThisBlock(text, sqlValStartExpression, sqlValEndExpression, Qt::darkRed );
        highlightThisBlock(text, txtValStartExpression, txtValEndExpression, Qt::darkGreen );
    }
    else
    {
        highlightThisBlock(text, sqlValStartExpression, sqlValEndExpression, QColor(204, 51, 153));
        highlightThisBlock(text, txtValStartExpression, txtValEndExpression, QColor(179, 137, 23) );
    }
}

void SqlHighlighter::highlightThisBlock(const QString &text, QRegularExpression startExp, QRegularExpression stopExp, QColor color)
{
    deb("highlightThisBlock start");
    QRegularExpressionMatch qrem;
    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegularExpression expression(rule.pattern);
        qrem = expression.match(text);
        int index = qrem.capturedStart();

        deb("highlightThisBlock hlRule index: "+GString(index));
        while (index >= 0)
        {
            int length = qrem.capturedLength();
            if( length <= 0 ) break;
            setFormat(index, length, rule.format);
            qrem = expression.match(text, index + length);
            index = qrem.capturedStart();
        }
    }
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
    {
        qrem = startExp.match(text);
        startIndex = qrem.capturedStart();
    }
    deb("highlightThisBlock start while...");
    while (startIndex >= 0)
    {
        qrem = stopExp.match(text, startIndex + 1);
        int endIndex = qrem.capturedStart();
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex  + qrem.capturedLength();
        }
        //setFormat(startIndex, commentLength, multiLineCommentFormat);
        setFormat(startIndex, commentLength, color);
        qrem = startExp.match(text, startIndex + commentLength);
        startIndex = qrem.capturedStart();
    }
    deb("highlightThisBlock done");

}
#else
/******************************************************************
 * QT 5
*******************************************************************/
SqlHighlighter::SqlHighlighter( PmfColorScheme colorScheme, QTextDocument *parent, GSeq<GString> *sqlCmdList, GSeq<GString> *hostVarList)  : QSyntaxHighlighter(parent)
{
    m_iColorScheme = colorScheme;
    HighlightingRule sqlSyntaxRule;
    HighlightingRule colNamesRule;

    if(!Helper::isSystemDarkPalette())
	{
		sqlKeywordFormat.setForeground(Qt::darkBlue);
		//sqlKeywordFormat.setFontWeight(QFont::Bold);
		colNameFormat.setForeground(Qt::darkGreen);
		//colNameFormat.setFontWeight(QFont::Bold);
	}
	else
	{
		sqlKeywordFormat.setForeground(Qt::cyan);
		colNameFormat.setForeground(Qt::magenta);
	}



    QStringList sqlKeywordPatterns;
    QStringList colNamesPatterns;


    if( hostVarList != NULL)
    {
        for( int i = 1; i <= (int)hostVarList->numberOfElements(); ++i)
        {
            if( !hostVarList->elementAtPosition(i).strip().length() ) continue;
            colNamesPatterns << "\\b"+hostVarList->elementAtPosition(i).strip("\"")+"\\b";
        }
    }

    if( sqlCmdList != NULL )
    {
        for( int i = 1; i <= (int)sqlCmdList->numberOfElements(); ++i)
        {
            if( !sqlCmdList->elementAtPosition(i).strip().length() ) continue;
            sqlKeywordPatterns << "\\b"+sqlCmdList->elementAtPosition(i)+"\\b";
        }

    }

    foreach (const QString &pattern, sqlKeywordPatterns)
    {
        sqlSyntaxRule.pattern = QRegExp(pattern,  Qt::CaseInsensitive);
        sqlSyntaxRule.format = sqlKeywordFormat;
        highlightingRules.append(sqlSyntaxRule);
    }

    foreach (const QString &pattern, colNamesPatterns)
    {
        colNamesRule.pattern = QRegExp(pattern,  Qt::CaseInsensitive);
        colNamesRule.format = colNameFormat;
        highlightingRules.append(colNamesRule);
    }
    /*
    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkMagenta);
    sqlSyntaxRule.pattern = QRegExp("\\b[A-Za-z0-9_]+\\b");
    sqlSyntaxRule.format = classFormat;
    highlightingRules.append(sqlSyntaxRule);

    quotationFormat.setForeground(Qt::darkGreen);
    sqlSyntaxRule.pattern = QRegExp("\".*\"");
    sqlSyntaxRule.format = quotationFormat;
    highlightingRules.append(sqlSyntaxRule);


    functionFormat.setFontItalic(true);
    functionFormat.setForeground(Qt::blue);
    sqlSyntaxRule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
    sqlSyntaxRule.format = functionFormat;
    highlightingRules.append(sqlSyntaxRule);
    */
    if(!Helper::isSystemDarkPalette()) singleLineCommentFormat.setForeground(Qt::darkGray);
	else singleLineCommentFormat.setForeground(Qt::gray);
    sqlSyntaxRule.pattern = QRegExp("--[^\n]*");
    sqlSyntaxRule.format = singleLineCommentFormat;
    highlightingRules.append(sqlSyntaxRule);

    if(!Helper::isSystemDarkPalette()) multiLineCommentFormat.setForeground(Qt::red);
	else multiLineCommentFormat.setForeground(Qt::darkGray);

    sqlValStartExpression= QRegExp("'");
    sqlValEndExpression= QRegExp("'");

    txtValStartExpression= QRegExp("\"");
    txtValEndExpression= QRegExp("\"");

    commentStartExpression = QRegExp("/\\*");
    commentEndExpression = QRegExp("\\*/");
}
void SqlHighlighter::deb(GString msg)
{
    return;
    printf("SqlHighlighter: %s\n", (char*) msg);
#if defined(MAKE_VC) || defined (__MINGW32__)
    _flushall();
#endif
}

void SqlHighlighter::highlightBlock(const QString &text)
{
    if( !Helper::isSystemDarkPalette() ) //Standard
    {
        highlightThisBlock(text, sqlValStartExpression, sqlValEndExpression, Qt::darkRed );
        highlightThisBlock(text, txtValStartExpression, txtValEndExpression, Qt::darkGreen );
    }
    else
    {
        highlightThisBlock(text, sqlValStartExpression, sqlValEndExpression, QColor(204, 51, 153));
        highlightThisBlock(text, txtValStartExpression, txtValEndExpression, QColor(179, 137, 23) );
    }
}

void SqlHighlighter::highlightThisBlock(const QString &text, QRegExp startExp, QRegExp stopExp, QColor color)
{
    deb("highlightThisBlock start");
    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        deb("highlightThisBlock hlRule index: "+GString(index));
        while (index >= 0)
        {
            int length = expression.matchedLength();
            if( length <= 0 ) break;
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1) startIndex = startExp.indexIn(text);
    deb("highlightThisBlock start while...");
    while (startIndex >= 0)
    {
        int endIndex = stopExp.indexIn(text, startIndex+1);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex  + stopExp.matchedLength();
        }
        //setFormat(startIndex, commentLength, multiLineCommentFormat);
        setFormat(startIndex, commentLength, color);
        startIndex = startExp.indexIn(text, startIndex + commentLength);
    }
    deb("highlightThisBlock done");

}
#endif


