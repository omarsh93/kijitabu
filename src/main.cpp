// main.cpp
#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QStatusBar>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QDir>
// 追加ヘッダ
#include <QSettings>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QPainter>
#include <QTextBlock>



class CodeEditor;

class LineNumberArea : public QWidget
{
public:
    //LineNumberArea(CodeEditor *editor)
    //    : QWidget(editor), codeEditor(editor)
    //{
    //}
    LineNumberArea(CodeEditor *editor)
    : QWidget(nullptr), codeEditor(editor)
    {
        setParent(reinterpret_cast<QWidget*>(editor));
    }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *codeEditor;
};

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
    {
        lineNumberArea = new LineNumberArea(this);

        connect(this, &CodeEditor::blockCountChanged,
                this, &CodeEditor::updateLineNumberAreaWidth);

        connect(this, &CodeEditor::updateRequest,
                this, &CodeEditor::updateLineNumberArea);

        connect(this, &CodeEditor::cursorPositionChanged,
                this, &CodeEditor::highlightCurrentLine);

        updateLineNumberAreaWidth(0);
        highlightCurrentLine();
    }

    int lineNumberAreaWidth()
    {
        int digits = 1;
        int max = qMax(1, blockCount());

        while (max >= 10)
        {
            max /= 10;
            ++digits;
        }

        int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

        return space;
    }

    void updateLineNumberAreaWidth(int)
    {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }

    void updateLineNumberArea(const QRect &rect, int dy)
    {
        if (dy)
        {
            lineNumberArea->scroll(0, dy);
        }
        else
        {
            lineNumberArea->update(0, rect.y(),
                                   lineNumberArea->width(),
                                   rect.height());
        }

        if (rect.contains(viewport()->rect()))
        {
            updateLineNumberAreaWidth(0);
        }
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QPlainTextEdit::resizeEvent(event);

        QRect cr = contentsRect();

        lineNumberArea->setGeometry(
            QRect(cr.left(),
                  cr.top(),
                  lineNumberAreaWidth(),
                  cr.height()));
    }

    void highlightCurrentLine()
    {
        QList<QTextEdit::ExtraSelection> extraSelections;

        QTextEdit::ExtraSelection selection;

        //QColor lineColor = QColor(Qt::lightGray).lighter(180);
        QColor lineColor = QColor(Qt::darkGray);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(
            QTextFormat::FullWidthSelection, true);

        selection.cursor = textCursor();
        selection.cursor.clearSelection();

        extraSelections.append(selection);

        setExtraSelections(extraSelections);
    }

    void lineNumberAreaPaintEvent(QPaintEvent *event)
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

                painter.setPen(Qt::black);

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

private:
    QWidget *lineNumberArea;

    friend class LineNumberArea;
};

QSize LineNumberArea::sizeHint() const
{
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    codeEditor->lineNumberAreaPaintEvent(event);
}

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow()
    {
        //textEdit = new QTextEdit(this);
        //textEdit = new QPlainTextEdit(this);
        textEdit = new CodeEditor(this);

        // 日本語入力(IME)
        textEdit->setAttribute(Qt::WA_InputMethodEnabled, true);

        setCentralWidget(textEdit);

        createMenus();

        resize(800, 600);
        //setWindowTitle("Kijitabu");
        updateWindowTitle();

        //loadAutoSave();
        restoreLastSession();

        statusBar()->showMessage("準備完了");
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        autoSave();
        event->accept();
    }

private:
    //QTextEdit *textEdit;
    QPlainTextEdit *textEdit;
    QString currentFile;
    QSettings settings {"kijitabu", "kijitabu"};

    void updateWindowTitle()
    {
        QString name;

        if (currentFile.isEmpty())
        {
            name = "無題";
        }
        else
        {
            name = QFileInfo(currentFile).fileName();
        }

        setWindowTitle(name + " - Kijitabu");
    }

    void restoreLastSession()
    {
        QString lastFile =
            settings.value("lastFile").toString();

        // 最後に開いていたファイルを復元
        if (!lastFile.isEmpty() && QFile::exists(lastFile))
        {
            if (loadFromFile(lastFile))
            {
                statusBar()->showMessage(
                    "前回のファイルを復元しました");
                return;
            }
        }

        // ファイルが無ければautosave復元
        loadAutoSave();
    }

    QString autoSavePath()
    {
        QString dir =
            QStandardPaths::writableLocation(
                QStandardPaths::AppDataLocation);

        QDir().mkpath(dir);

        return dir + "/autosave.txt";
    }

    void createMenus()
    {
        auto *fileMenu = menuBar()->addMenu("ファイル");

        auto *newAction  = fileMenu->addAction("新規");
        auto *openAction = fileMenu->addAction("開く");
        auto *saveAction = fileMenu->addAction("保存");
        auto *saveAsAction = fileMenu->addAction("名前を付けて保存");
        auto *exitAction = fileMenu->addAction("終了");

        connect(newAction, &QAction::triggered,
                this, &EditorWindow::newFile);

        connect(openAction, &QAction::triggered,
                this, &EditorWindow::openFile);

        connect(saveAction, &QAction::triggered,
                this, &EditorWindow::saveFile);

        connect(saveAsAction, &QAction::triggered,
                this, &EditorWindow::saveFileAs);

        connect(exitAction, &QAction::triggered,
                this, &QWidget::close);
    }

    void newFile()
    {
        currentFile.clear();
        textEdit->clear();
        settings.remove("lastFile");

        statusBar()->showMessage("新規ファイル");
        updateWindowTitle();
    }

    void openFile()
    {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "ファイルを開く"
        );

        if (fileName.isEmpty())
            return;

        loadFromFile(fileName);
    }

    bool loadFromFile(const QString &fileName)
    {
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::warning(this,
                                 "エラー",
                                 "ファイルを開けません");
            return false;
        }

        QTextStream in(&file);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        in.setCodec("UTF-8");
#else
        in.setEncoding(QStringConverter::Utf8);
#endif

        textEdit->setPlainText(in.readAll());

        currentFile = fileName;
        updateWindowTitle();
        settings.setValue("lastFile", fileName);

        statusBar()->showMessage("読み込み完了");

        return true;
    }

    bool saveToFile(const QString &fileName)
    {
        QFile file(fileName);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::warning(this,
                                 "エラー",
                                 "保存できません");
            return false;
        }

        QTextStream out(&file);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        out.setCodec("UTF-8");
#else
        out.setEncoding(QStringConverter::Utf8);
#endif

        out << textEdit->toPlainText();

        currentFile = fileName;
        updateWindowTitle();
        settings.setValue("lastFile", fileName);

        statusBar()->showMessage("保存完了");

        return true;
    }

    void saveFile()
    {
        if (currentFile.isEmpty())
        {
            saveFileAs();
            return;
        }

        saveToFile(currentFile);
    }

    void saveFileAs()
    {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "ファイルを保存"
        );

        if (fileName.isEmpty())
            return;

        saveToFile(fileName);
    }

    // 自動保存
    void autoSave()
    {
        QFile file(autoSavePath());

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

        QTextStream out(&file);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        out.setCodec("UTF-8");
#else
        out.setEncoding(QStringConverter::Utf8);
#endif

        out << textEdit->toPlainText();
    }

    // 起動時復元
    void loadAutoSave()
    {
        QFile file(autoSavePath());

        if (!file.exists())
            return;

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        QTextStream in(&file);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        in.setCodec("UTF-8");
#else
        in.setEncoding(QStringConverter::Utf8);
#endif

        textEdit->setPlainText(in.readAll());

        statusBar()->showMessage("前回の内容を復元しました");
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //QApplication::setApplicationName("QtEditor");
    //QApplication::setOrganizationName("Local");
    QApplication::setApplicationName("kijitabu");
    //QApplication::setOrganizationName("kijitabu");
    QApplication::setOrganizationName("");

    EditorWindow window;
    window.show();

    return app.exec();
}
