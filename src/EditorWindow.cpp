#include "EditorWindow.h"
#include "CodeEditor.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QStatusBar>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFontDialog>

EditorWindow::EditorWindow()
{
    textEdit = new CodeEditor(this);

    textEdit->setAttribute(Qt::WA_InputMethodEnabled, true);

    setCentralWidget(textEdit);

    createMenus();

    resize(800, 600);

    QString fontStr = settings.value("font").toString();
    if (!fontStr.isEmpty())
    {
        QFont savedFont;
        savedFont.fromString(fontStr);
        applyFont(savedFont);
    }

    updateWindowTitle();

    restoreLastSession();

    statusBar()->showMessage("準備完了");
}

void EditorWindow::closeEvent(QCloseEvent *event)
{
    autoSave();

    event->accept();
}

void EditorWindow::updateWindowTitle()
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

    void EditorWindow::restoreLastSession()
    {
        // autosaveを復元（閉じる時に常にautosaveしている）
        loadAutoSave();
    }

QString EditorWindow::autoSavePath()
{
    QString dir =
        QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);

    QDir().mkpath(dir);

    return dir + "/autosave.txt";
}

void EditorWindow::createMenus()
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

        // Ctrl+N 新規
        newAction->setShortcut(QKeySequence::New);

        // Ctrl+O 開く
        openAction->setShortcut(QKeySequence::Open);

        // Ctrl+S 保存
        saveAction->setShortcut(QKeySequence::Save);

        // Ctrl+Shift+S 名前を付けて保存
        saveAsAction->setShortcut(QKeySequence::SaveAs);

        // Ctrl+Q 終了
        exitAction->setShortcut(QKeySequence::Quit);

        auto *viewMenu = menuBar()->addMenu("表示");

        auto *fontAction = viewMenu->addAction("フォント...");

        connect(fontAction, &QAction::triggered,
                this, &EditorWindow::selectFont);
    }

    void EditorWindow::newFile()
    {
        currentFile.clear();
        textEdit->clear();
        settings.remove("lastFile");

        statusBar()->showMessage("新規ファイル");
        updateWindowTitle();
    }

    void EditorWindow::openFile()
    {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "ファイルを開く"
        );

        if (fileName.isEmpty())
            return;

        loadFromFile(fileName);
    }

    bool EditorWindow::loadFromFile(const QString &fileName)
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

    bool EditorWindow::saveToFile(const QString &fileName)
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

    void EditorWindow::saveFile()
    {
        if (currentFile.isEmpty())
        {
            saveFileAs();
            return;
        }

        saveToFile(currentFile);
    }

    void EditorWindow::saveFileAs()
    {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "ファイルを保存"
        );

        if (fileName.isEmpty())
            return;

        saveToFile(fileName);
    }

    void EditorWindow::selectFont()
    {
        bool ok;
        QFont font = QFontDialog::getFont(
            &ok, textEdit->font(), this, "フォントを選択");

        if (ok)
        {
            applyFont(font);
            settings.setValue("font", font.toString());
        }
    }

    void EditorWindow::applyFont(const QFont &font)
    {
        textEdit->setFont(font);
        static_cast<CodeEditor*>(textEdit)->updateLineNumberAreaWidth(0);
    }

    // 自動保存
    void EditorWindow::autoSave()
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
    void EditorWindow::loadAutoSave()
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
