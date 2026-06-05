#include "CodeEditor.h"
#include "LineNumberArea.h"

#include <QPainter>
#include <QTextBlock>
#include <QTextDocument>
#include <QPaintEvent>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);

    connect(this, &CodeEditor::updateRequest,
            this, &CodeEditor::updateLineNumberArea);

    connect(this, &CodeEditor::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    setCursorWidth(0);

    // 検索マッチ選択時も黄色で表示されるよう選択色を設定
    QPalette p = palette();
    p.setColor(QPalette::Highlight, QColor(255, 220, 0));
    p.setColor(QPalette::HighlightedText, Qt::black);
    setPalette(p);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    // カーソル（赤い棒）
    QPainter painter(viewport());
    painter.setClipRect(event->rect());
    if (!isReadOnly())
    {
        QTextCursor cursor = textCursor();
        QRect cursorR = cursorRect(cursor);
        cursorR.setWidth(3);
        painter.fillRect(cursorR, QColor(255, 80, 80));
    }
}


int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());

    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    int space =
        3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    space += 10;

    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
    {
        lineNumberArea->scroll(0, dy);
    }
    else
    {
        lineNumberArea->update(
            0,
            rect.y(),
            lineNumberArea->width(),
            rect.height());
    }

    if (rect.contains(viewport()->rect()))
    {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();

    lineNumberArea->setGeometry(
        QRect(cr.left(),
              cr.top(),
              lineNumberAreaWidth(),
              cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    currentLineSelections.clear();

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(105, 105, 105, 60));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        currentLineSelections.append(selection);
    }

    mergeAndApplySelections();
}

void CodeEditor::mergeAndApplySelections()
{
    setExtraSelections(currentLineSelections + searchSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);

    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();

    int blockNumber = block.blockNumber();

    int top =
        qRound(blockBoundingGeometry(block)
               .translated(contentOffset()).top());

    int bottom =
        top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() &&
            bottom >= event->rect().top())
        {
            QString number =
                QString::number(blockNumber + 1);

            //painter.setPen(Qt::black);
            
            if (blockNumber == textCursor().blockNumber())
            {
                QFont bold = painter.font();
                bold.setBold(true);
                painter.setFont(bold);

                painter.setPen(QColor(60, 60, 60));
            }
            else
            {
                QFont normal = painter.font();
                normal.setBold(false);
                painter.setFont(normal);

                painter.setPen(Qt::black);
            }

            painter.drawText(
                0,
                top,
                lineNumberArea->width() - 5,
                fontMetrics().height(),
                Qt::AlignRight,
                number);
        }

        block = block.next();

        top = bottom;

        bottom =
            top + qRound(blockBoundingRect(block).height());

        ++blockNumber;
    }
}

void CodeEditor::setSearchHighlight(const QString &text,
                                    QTextDocument::FindFlags flags)
{
    searchSelections.clear();

    if (!text.isEmpty())
    {
        QTextCursor cursor(document());
        while (!cursor.isNull() && !cursor.atEnd())
        {
            cursor = document()->find(text, cursor, flags);
            if (!cursor.isNull())
            {
                QTextEdit::ExtraSelection sel;
                sel.cursor = cursor;
                sel.format.setBackground(QColor(255, 255, 0));
                searchSelections.append(sel);
            }
        }
    }

    mergeAndApplySelections();
}

void CodeEditor::clearSearchHighlight()
{
    searchSelections.clear();
    mergeAndApplySelections();
}
