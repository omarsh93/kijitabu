#include "CodeEditor.h"
#include "LineNumberArea.h"

#include <QPainter>
#include <QTextBlock>
#include <QPainter>
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

    //setOverwriteMode(true);
    
    
   
    
    
    // 追加
    //setCursorWidth(4);
    setCursorWidth(0);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::paintEvent(QPaintEvent *event)
{
    // 通常描画
    QPlainTextEdit::paintEvent(event);

    QPainter painter(viewport());

    QTextCursor cursor = textCursor();

    QRect rect = cursorRect(cursor);

    // カーソル幅（カーソル位置の文字の実際の幅に合わせる）
    QString ch = cursor.block().text().mid(
        cursor.positionInBlock(), 1);
    int w = ch.isEmpty()
        ? fontMetrics().horizontalAdvance('M')
        : fontMetrics().horizontalAdvance(ch);
    rect.setWidth(w);

    // 半透明水色
    QColor cursorColor(0, 255, 255, 120);

    painter.fillRect(rect, cursorColor);

    /*
    // 文字を再描画
    QString ch = cursor.block().text().mid(
        cursor.positionInBlock(),
        1
    );

    if (!ch.isEmpty())
    {
        painter.setPen(Qt::black);

        painter.drawText(
            rect,
            Qt::AlignCenter,
            ch
        );
    }
    */
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
    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor(Qt::darkGray);

    selection.format.setBackground(lineColor);

    selection.format.setProperty(
        QTextFormat::FullWidthSelection,
        true);

    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    extraSelections.append(selection);

    setExtraSelections(extraSelections);
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
