#include "LineNumberArea.h"
#include "CodeEditor.h"

LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(nullptr),
      codeEditor(editor)
{
    setParent(reinterpret_cast<QWidget*>(editor));
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    codeEditor->lineNumberAreaPaintEvent(event);
}
