#include "EditorWindow.h"
#include "CodeEditor.h"

#include <QApplication>
#include <QWindow>
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

    recentFiles = settings.value("recentFiles").toStringList();
    updateRecentFilesMenu();

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
    // 前回開いていたファイルを復元
    QString lastFile = settings.value("lastFile").toString();
    if (!lastFile.isEmpty())
    {
        loadFromFile(lastFile);
    }

    // autosaveを復元（閉じる時に常にautosaveしている）
    loadAutoSave();

    // カーソル位置を復元
    int cursorPos = settings.value("cursorPosition", 0).toInt();
    if (cursorPos > 0)
    {
        QTextCursor cursor = textEdit->textCursor();
        cursor.setPosition(qMin(cursorPos, textEdit->document()->characterCount() - 1));
        textEdit->setTextCursor(cursor);
        textEdit->ensureCursorVisible();
    }
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

        fileMenu->addSeparator();
        recentFilesMenu = fileMenu->addMenu("最近のファイル");

        fileMenu->addSeparator();
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

        auto *editMenu = menuBar()->addMenu("編集");

        auto *findAction = editMenu->addAction("検索...");
        auto *findNextAction = editMenu->addAction("次を検索");
        auto *findPrevAction = editMenu->addAction("前を検索");

        connect(findAction, &QAction::triggered,
                this, &EditorWindow::showFindDialog);

        connect(findNextAction, &QAction::triggered,
                this, &EditorWindow::findNext);

        connect(findPrevAction, &QAction::triggered,
                this, &EditorWindow::findPrevious);

        findAction->setShortcut(QKeySequence::Find);
        findNextAction->setShortcut(QKeySequence(Qt::Key_F3));
        findPrevAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F3));
    }

void EditorWindow::addToRecentFiles(const QString &fileName)
{
    recentFiles.removeOne(fileName);
    recentFiles.prepend(fileName);

    while (recentFiles.size() > maxRecentFiles)
        recentFiles.removeLast();

    settings.setValue("recentFiles", recentFiles);
    updateRecentFilesMenu();
}

void EditorWindow::updateRecentFilesMenu()
{
    recentFilesMenu->clear();

    for (const auto &file : recentFiles)
    {
        if (file == untitledSentinel)
        {
            auto *action = recentFilesMenu->addAction("無題");
            connect(action, &QAction::triggered, this, &EditorWindow::newFile);
        }
        else
        {
            auto *action = recentFilesMenu->addAction(file);
            connect(action, &QAction::triggered, this, [this, file]() {
                loadFromFile(file);
            });
        }
    }

    recentFilesMenu->setEnabled(!recentFiles.isEmpty());
}

void EditorWindow::newFile()
{
    currentFile.clear();
    textEdit->clear();
    lastSearch.clear();
    static_cast<CodeEditor*>(textEdit)->clearSearchHighlight();
    settings.remove("lastFile");
    settings.remove("cursorPosition");

    addToRecentFiles(untitledSentinel);

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

        addToRecentFiles(fileName);

        lastSearch.clear();
        static_cast<CodeEditor*>(textEdit)->clearSearchHighlight();

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

        addToRecentFiles(fileName);

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

    // カーソル位置を保存
    settings.setValue("cursorPosition", textEdit->textCursor().position());
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

    void EditorWindow::showFindDialog()
    {
        QDialog dialog(this);
        dialog.setWindowTitle("検索");

        auto *layout = new QVBoxLayout(&dialog);

        auto *searchInput = new QLineEdit(&dialog);
        searchInput->setPlaceholderText("検索する文字列");
        if (!lastSearch.isEmpty())
            searchInput->setText(lastSearch);
        layout->addWidget(searchInput);

        auto *caseCheck = new QCheckBox("大文字/小文字を区別", &dialog);
        layout->addWidget(caseCheck);

        auto *buttonLayout = new QHBoxLayout();
        auto *findNextBtn = new QPushButton("次を検索", &dialog);
        auto *findPrevBtn = new QPushButton("前を検索", &dialog);
        auto *closeBtn = new QPushButton("閉じる", &dialog);
        buttonLayout->addWidget(findNextBtn);
        buttonLayout->addWidget(findPrevBtn);
        buttonLayout->addWidget(closeBtn);
        layout->addLayout(buttonLayout);

        auto applyHighlight = [&]() {
            auto *editor = static_cast<CodeEditor*>(textEdit);
            editor->setSearchHighlight(lastSearch, lastSearchFlags);
        };

        auto doFind = [&](QTextDocument::FindFlags flags) {
            QString text = searchInput->text();
            if (text.isEmpty()) return;

            lastSearch = text;
            lastSearchFlags = {};
            if (caseCheck->isChecked())
                lastSearchFlags |= QTextDocument::FindCaseSensitively;
            flags |= lastSearchFlags;

            if (textEdit->find(text, flags))
            {
                applyHighlight();
                return;
            }

            QTextCursor cursor(textEdit->document());
            if (flags & QTextDocument::FindBackward)
                cursor.movePosition(QTextCursor::End);
            textEdit->setTextCursor(cursor);

            if (textEdit->find(text, flags))
            {
                applyHighlight();
                statusBar()->showMessage("検索文字列が見つかりました（先頭／末尾から続き）");
            }
            else
            {
                applyHighlight();
                statusBar()->showMessage("見つかりませんでした");
            }
        };

        connect(findNextBtn, &QPushButton::clicked, [&]() {
            doFind(QTextDocument::FindFlags());
        });

        connect(findPrevBtn, &QPushButton::clicked, [&]() {
            doFind(QTextDocument::FindBackward);
        });

        connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::close);

        connect(searchInput, &QLineEdit::returnPressed, [&]() {
            doFind(QTextDocument::FindFlags());
        });

        searchInput->selectAll();
        searchInput->setFocus();
        dialog.exec();
    }

    void EditorWindow::findNext()
    {
        if (lastSearch.isEmpty())
        {
            showFindDialog();
            return;
        }

        if (textEdit->find(lastSearch, lastSearchFlags))
            return;

        QTextCursor cursor(textEdit->document());
        cursor.movePosition(QTextCursor::Start);
        textEdit->setTextCursor(cursor);

        if (textEdit->find(lastSearch, lastSearchFlags))
            statusBar()->showMessage("検索文字列が見つかりました（先頭から続き）");
        else
            statusBar()->showMessage("見つかりませんでした");
    }

    void EditorWindow::findPrevious()
    {
        if (lastSearch.isEmpty())
        {
            showFindDialog();
            return;
        }

        QTextDocument::FindFlags flags = QTextDocument::FindBackward | lastSearchFlags;

        if (textEdit->find(lastSearch, flags))
            return;

        QTextCursor cursor(textEdit->document());
        cursor.movePosition(QTextCursor::End);
        textEdit->setTextCursor(cursor);

        if (textEdit->find(lastSearch, flags))
            statusBar()->showMessage("検索文字列が見つかりました（末尾から続き）");
        else
            statusBar()->showMessage("見つかりませんでした");
    }

    void EditorWindow::bringToFront()
    {
        showNormal();
        raise();
        activateWindow();
        if (auto *win = windowHandle())
            win->requestActivate();
        QApplication::alert(this, 3000);
    }
