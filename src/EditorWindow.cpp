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
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    setCentralWidget(tabWidget);

    connect(tabWidget, &QTabWidget::tabCloseRequested,
            this, &EditorWindow::onTabCloseRequest);
    connect(tabWidget, &QTabWidget::currentChanged,
            this, &EditorWindow::onTabChanged);

    createMenus();

    recentFiles = settings.value("recentFiles").toStringList();
    updateRecentFilesMenu();

    resize(800, 600);

    QString fontStr = settings.value("font").toString();
    if (!fontStr.isEmpty())
    {
        currentFont.fromString(fontStr);
    }
    else
    {
        currentFont = QFont();
    }

    restoreLastSession();

    statusBar()->showMessage("準備完了");
}

CodeEditor* EditorWindow::currentEditor() const
{
    return qobject_cast<CodeEditor*>(tabWidget->currentWidget());
}

int EditorWindow::addNewTab(const QString &title)
{
    auto *editor = new CodeEditor();
    editor->setAttribute(Qt::WA_InputMethodEnabled, true);
    editor->setFont(currentFont);
    editor->updateLineNumberAreaWidth(0);

    connect(editor->document(), &QTextDocument::modificationChanged,
            this, [this, editor]() {
        int idx = tabWidget->indexOf(editor);
        if (idx >= 0) updateTabTitle(idx);
    });

    QString tabTitle = title.isEmpty() ? "無題" : title;
    int idx = tabWidget->addTab(editor, tabTitle);
    tabFiles[editor] = QString();
    tabWidget->setCurrentIndex(idx);
    return idx;
}

void EditorWindow::closeTab(int index)
{
    auto *editor = qobject_cast<CodeEditor*>(tabWidget->widget(index));
    if (!editor) return;

    tabFiles.remove(editor);
    tabWidget->removeTab(index);

    if (tabWidget->count() == 0)
    {
        addNewTab("無題");
    }

    updateWindowTitle();
}

bool EditorWindow::maybeSaveTab(int index)
{
    auto *editor = qobject_cast<CodeEditor*>(tabWidget->widget(index));
    if (!editor || !editor->document()->isModified()) return true;

    QString path = tabFiles.value(editor);
    QString name = path.isEmpty() ? "無題" : QFileInfo(path).fileName();

    auto ret = QMessageBox::warning(this, "Kijitabu",
        name + " は変更されています。保存しますか？",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
    {
        if (path.isEmpty())
        {
            QString fileName = QFileDialog::getSaveFileName(this, "ファイルを保存");
            if (fileName.isEmpty()) return false;
            return saveToFile(fileName, editor);
        }
        return saveToFile(path, editor);
    }

    return ret == QMessageBox::Discard;
}

void EditorWindow::updateTabTitle(int index)
{
    auto *editor = qobject_cast<CodeEditor*>(tabWidget->widget(index));
    if (!editor) return;

    QString path = tabFiles.value(editor);
    QString name = path.isEmpty() ? "無題" : QFileInfo(path).fileName();
    if (editor->document()->isModified())
        name = "* " + name;
    tabWidget->setTabText(index, name);
}

void EditorWindow::updateWindowTitle()
{
    auto *editor = currentEditor();
    QString name;
    if (editor)
    {
        QString path = tabFiles.value(editor);
        if (path.isEmpty())
            name = "無題";
        else
            name = QFileInfo(path).fileName();
    }
    setWindowTitle(name + " - Kijitabu");
}

void EditorWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < tabWidget->count(); ++i)
    {
        if (!maybeSaveTab(i))
        {
            event->ignore();
            return;
        }
    }

    autoSaveAll();
    event->accept();
}

void EditorWindow::restoreLastSession()
{
    int count = settings.value("tabCount", 0).toInt();
    QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);

    if (count > 0)
    {
        settings.beginReadArray("tabs");
        for (int i = 0; i < count; ++i)
        {
            settings.setArrayIndex(i);
            QString filePath = settings.value("filePath").toString();
            int cursorPos = settings.value("cursorPosition", 0).toInt();

            auto *editor = new CodeEditor();
            editor->setAttribute(Qt::WA_InputMethodEnabled, true);
            editor->setFont(currentFont);
            editor->updateLineNumberAreaWidth(0);

            QString autoFile = dir + QString("/autosave_%1.txt").arg(i);
            QFile af(autoFile);
            if (af.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QTextStream in(&af);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
                in.setCodec("UTF-8");
#else
                in.setEncoding(QStringConverter::Utf8);
#endif

                editor->setPlainText(in.readAll());
            }

            connect(editor->document(), &QTextDocument::modificationChanged,
                    this, [this, editor]() {
                int idx = tabWidget->indexOf(editor);
                if (idx >= 0) updateTabTitle(idx);
            });

            QString tabName = filePath.isEmpty() ? "無題" : QFileInfo(filePath).fileName();
            tabWidget->addTab(editor, tabName);
            tabFiles[editor] = filePath;

            if (cursorPos > 0)
            {
                QTextCursor cursor = editor->textCursor();
                cursor.setPosition(qMin(cursorPos, editor->document()->characterCount() - 1));
                editor->setTextCursor(cursor);
                editor->ensureCursorVisible();
            }
        }
        settings.endArray();

        if (tabWidget->count() > 0)
            tabWidget->setCurrentIndex(0);
    }
    else
    {
        addNewTab("無題");

        QFile file(dir + "/autosave.txt");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&file);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
            in.setCodec("UTF-8");
#else
            in.setEncoding(QStringConverter::Utf8);
#endif

            auto *editor = currentEditor();
            if (editor)
            {
                editor->setPlainText(in.readAll());
                statusBar()->showMessage("前回の内容を復元しました");
            }
        }

        int cursorPos = settings.value("cursorPosition", 0).toInt();
        auto *editor = currentEditor();
        if (editor && cursorPos > 0)
        {
            QTextCursor cursor = editor->textCursor();
            cursor.setPosition(qMin(cursorPos, editor->document()->characterCount() - 1));
            editor->setTextCursor(cursor);
            editor->ensureCursorVisible();
        }
    }
}

void EditorWindow::autoSaveAll()
{
    QString dir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);

    int count = tabWidget->count();
    settings.beginWriteArray("tabs");
    for (int i = 0; i < count; ++i)
    {
        auto *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (!editor) continue;

        settings.setArrayIndex(i);
        QString path = tabFiles.value(editor);
        settings.setValue("filePath", path);

        QFile file(dir + QString("/autosave_%1.txt").arg(i));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
            out.setCodec("UTF-8");
#else
            out.setEncoding(QStringConverter::Utf8);
#endif

            out << editor->toPlainText();
        }

        settings.setValue("cursorPosition",
                          editor->textCursor().position());
    }
    settings.endArray();
    settings.setValue("tabCount", count);
}

void EditorWindow::createMenus()
{
    auto *fileMenu = menuBar()->addMenu("ファイル");

    auto *newAction  = fileMenu->addAction("新規");
    auto *openAction = fileMenu->addAction("開く");
    auto *saveAction = fileMenu->addAction("保存");
    auto *saveAsAction = fileMenu->addAction("名前を付けて保存");

    fileMenu->addSeparator();

    auto *closeTabAction = fileMenu->addAction("タブを閉じる");

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

    connect(closeTabAction, &QAction::triggered,
            this, &EditorWindow::closeCurrentTab);

    connect(exitAction, &QAction::triggered,
            this, &QWidget::close);

    newAction->setShortcut(QKeySequence::New);
    openAction->setShortcut(QKeySequence::Open);
    saveAction->setShortcut(QKeySequence::Save);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    closeTabAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
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
                openFileAtPath(file);
            });
        }
    }

    recentFilesMenu->setEnabled(!recentFiles.isEmpty());
}

void EditorWindow::newFile()
{
    addNewTab("無題");
    addToRecentFiles(untitledSentinel);

    statusBar()->showMessage("新規ファイル");
}

void EditorWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "ファイルを開く"
    );

    if (fileName.isEmpty())
        return;

    openFileAtPath(fileName);
}

void EditorWindow::openFileAtPath(const QString &fileName)
{
    for (auto it = tabFiles.begin(); it != tabFiles.end(); ++it)
    {
        if (it.value() == fileName)
        {
            tabWidget->setCurrentWidget(it.key());
            statusBar()->showMessage("読み込み完了");
            return;
        }
    }

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

    auto *editor = new CodeEditor();
    editor->setAttribute(Qt::WA_InputMethodEnabled, true);
    editor->setFont(currentFont);
    editor->updateLineNumberAreaWidth(0);
    editor->setPlainText(in.readAll());
    editor->document()->setModified(false);

    connect(editor->document(), &QTextDocument::modificationChanged,
            this, [this, editor]() {
        int idx = tabWidget->indexOf(editor);
        if (idx >= 0) updateTabTitle(idx);
    });

    QString name = QFileInfo(fileName).fileName();
    int idx = tabWidget->addTab(editor, name);
    tabFiles[editor] = fileName;
    tabWidget->setCurrentIndex(idx);

    updateWindowTitle();
    addToRecentFiles(fileName);

    lastSearch.clear();
    editor->clearSearchHighlight();

    statusBar()->showMessage("読み込み完了");

    return true;
}

bool EditorWindow::saveToFile(const QString &fileName, CodeEditor *editor)
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

    out << editor->toPlainText();

    tabFiles[editor] = fileName;
    editor->document()->setModified(false);

    int idx = tabWidget->indexOf(editor);
    if (idx >= 0) updateTabTitle(idx);

    updateWindowTitle();
    settings.setValue("lastFile", fileName);

    addToRecentFiles(fileName);

    statusBar()->showMessage("保存完了");

    return true;
}

void EditorWindow::saveFile()
{
    auto *editor = currentEditor();
    if (!editor) return;

    QString path = tabFiles.value(editor);
    if (path.isEmpty())
    {
        saveFileAs();
        return;
    }

    saveToFile(path, editor);
}

void EditorWindow::saveFileAs()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "ファイルを保存"
    );

    if (fileName.isEmpty())
        return;

    auto *editor = currentEditor();
    if (!editor) return;

    saveToFile(fileName, editor);
}

void EditorWindow::closeCurrentTab()
{
    int idx = tabWidget->currentIndex();
    if (idx < 0) return;

    if (tabWidget->count() <= 1)
    {
        if (!maybeSaveTab(idx)) return;
        auto *editor = currentEditor();
        if (editor)
        {
            editor->clear();
            editor->document()->setModified(false);
            tabFiles[editor] = QString();
            updateTabTitle(idx);
            updateWindowTitle();
        }
        return;
    }

    closeTab(idx);
}

void EditorWindow::selectFont()
{
    auto *editor = currentEditor();
    if (!editor) return;

    bool ok;
    QFont font = QFontDialog::getFont(
        &ok, editor->font(), this, "フォントを選択");

    if (ok)
    {
        currentFont = font;
        applyFontToAllEditors();
        settings.setValue("font", font.toString());
    }
}

void EditorWindow::applyFontToEditor(CodeEditor *editor)
{
    if (!editor) return;
    editor->setFont(currentFont);
    editor->updateLineNumberAreaWidth(0);
}

void EditorWindow::applyFontToAllEditors()
{
    for (int i = 0; i < tabWidget->count(); ++i)
    {
        auto *editor = qobject_cast<CodeEditor*>(tabWidget->widget(i));
        if (editor)
        {
            editor->setFont(currentFont);
            editor->updateLineNumberAreaWidth(0);
        }
    }
}

void EditorWindow::showFindDialog()
{
    auto *editor = currentEditor();
    if (!editor) return;

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
        auto *e = static_cast<CodeEditor*>(this->currentEditor());
        if (e) e->setSearchHighlight(lastSearch, lastSearchFlags);
    };

    auto doFind = [&](QTextDocument::FindFlags flags) {
        QString text = searchInput->text();
        if (text.isEmpty()) return;

        lastSearch = text;
        lastSearchFlags = {};
        if (caseCheck->isChecked())
            lastSearchFlags |= QTextDocument::FindCaseSensitively;
        flags |= lastSearchFlags;

        auto *e = currentEditor();
        if (!e) return;

        if (e->find(text, flags))
        {
            applyHighlight();
            return;
        }

        QTextCursor cursor(e->document());
        if (flags & QTextDocument::FindBackward)
            cursor.movePosition(QTextCursor::End);
        e->setTextCursor(cursor);

        if (e->find(text, flags))
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
    auto *editor = currentEditor();
    if (!editor) return;

    if (lastSearch.isEmpty())
    {
        showFindDialog();
        return;
    }

    if (editor->find(lastSearch, lastSearchFlags))
        return;

    QTextCursor cursor(editor->document());
    cursor.movePosition(QTextCursor::Start);
    editor->setTextCursor(cursor);

    if (editor->find(lastSearch, lastSearchFlags))
        statusBar()->showMessage("検索文字列が見つかりました（先頭から続き）");
    else
        statusBar()->showMessage("見つかりませんでした");
}

void EditorWindow::findPrevious()
{
    auto *editor = currentEditor();
    if (!editor) return;

    if (lastSearch.isEmpty())
    {
        showFindDialog();
        return;
    }

    QTextDocument::FindFlags flags = QTextDocument::FindBackward | lastSearchFlags;

    if (editor->find(lastSearch, flags))
        return;

    QTextCursor cursor(editor->document());
    cursor.movePosition(QTextCursor::End);
    editor->setTextCursor(cursor);

    if (editor->find(lastSearch, flags))
        statusBar()->showMessage("検索文字列が見つかりました（末尾から続き）");
    else
        statusBar()->showMessage("見つかりませんでした");
}

void EditorWindow::onTabCloseRequest(int index)
{
    if (tabWidget->count() <= 1)
    {
        if (!maybeSaveTab(index)) return;
        auto *editor = qobject_cast<CodeEditor*>(tabWidget->widget(index));
        if (editor)
        {
            editor->clear();
            editor->document()->setModified(false);
            tabFiles[editor] = QString();
            updateTabTitle(index);
            updateWindowTitle();
        }
        return;
    }

    closeTab(index);
}

void EditorWindow::onTabChanged(int index)
{
    Q_UNUSED(index);
    updateWindowTitle();

    auto *editor = currentEditor();
    if (editor)
    {
        editor->setFocus();
    }
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
