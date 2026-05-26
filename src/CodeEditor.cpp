#include "CodeEditor.h"
#include "LineNumberArea.h"

#include <QPainter>
#include <QTextBlock>
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

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    QPainter painter(viewport());
    painter.setClipRect(event->rect());

    // 現在行のハイライト（半透明なので選択範囲が見える）
    QTextCursor cursor = textCursor();
    QRect lineRect = cursorRect(cursor);
    lineRect.setLeft(0);
    lineRect.setRight(viewport()->width());
    painter.fillRect(lineRect, QColor(105, 105, 105, 60));

    // カーソル（赤い棒）
    if (!isReadOnly())
    {
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
    viewport()->update();
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

                painter.setPen(Qt::yellow);
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
