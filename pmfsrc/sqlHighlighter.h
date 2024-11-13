#ifndef SQLHIGHLIGHTER_H
#define SQLHIGHLIGHTER_H


#include<QSyntaxHighlighter>
#include <gseq.hpp>
#include <gstring.hpp>
#include <pmfdefines.h>

#if QT_VERSION >= 0x060000
#include <QRegularExpression>
#endif

class SqlHighlighter : public QSyntaxHighlighter
{


public:
    SqlHighlighter( PmfColorScheme colorScheme, QTextDocument *parent = 0, GSeq<GString> *sqlCmdList = NULL, GSeq<GString> *hostVarList = NULL);

protected:
    void highlightBlock(const QString &text);

private:

#if QT_VERSION >= 0x060000
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    PmfColorScheme m_iColorScheme;
    QVector<HighlightingRule> highlightingRules;
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;
    QRegularExpression sqlValStartExpression;
    QRegularExpression sqlValEndExpression;
    QRegularExpression txtValStartExpression;
    QRegularExpression txtValEndExpression;
    void highlightThisBlock(const QString &text, QRegularExpression startExp, QRegularExpression stopExp, QColor color);

#else
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    PmfColorScheme m_iColorScheme;
    QVector<HighlightingRule> highlightingRules;
    QRegExp commentStartExpression;
    QRegExp commentEndExpression;
    QRegExp sqlValStartExpression;
    QRegExp sqlValEndExpression;
    QRegExp txtValStartExpression;
    QRegExp txtValEndExpression;
    void highlightThisBlock(const QString &text, QRegExp startExp, QRegExp stopExp, QColor color);
#endif
    QTextCharFormat sqlKeywordFormat;
    QTextCharFormat colNameFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;


    void deb(GString msg);
};

#endif // SQLHIGHLIGHTER_H
