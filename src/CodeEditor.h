#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth();

    void lineNumberAreaPaintEvent(QPaintEvent *event);

    void setSearchHighlight(const QString &text,
                            QTextDocument::FindFlags flags = QTextDocument::FindFlags());
    void clearSearchHighlight();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

public slots:
    void updateLineNumberAreaWidth(int);

private slots:
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    QWidget *lineNumberArea;
    QList<QTextEdit::ExtraSelection> currentLineSelections;
    QList<QTextEdit::ExtraSelection> searchSelections;

    void mergeAndApplySelections();
};

#endif
