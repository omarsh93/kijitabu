#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QMap>
#include <QMenu>
#include <QSettings>
#include <QFont>
#include <QStringList>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextDocument>

class QPlainTextEdit;
class CodeEditor;

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QTabWidget *tabWidget;
    QMap<QPlainTextEdit*, QString> tabFiles;

    QMenu *recentFilesMenu;
    QSettings settings {"kijitabu", "kijitabu"};

    QStringList recentFiles;
    QString lastSearch;
    QTextDocument::FindFlags lastSearchFlags;
    QFont currentFont;
    static constexpr int maxRecentFiles = 10;
    static constexpr const char *untitledSentinel = "__untitled__";

    void createMenus();
    void updateRecentFilesMenu();
    void addToRecentFiles(const QString &fileName);

    void updateWindowTitle();

    void applyFontToEditor(CodeEditor *editor);
    void applyFontToAllEditors();

    void restoreLastSession();

    CodeEditor *currentEditor() const;
    int addNewTab(const QString &title = QString());
    void closeTab(int index);
    bool maybeSaveTab(int index);
    void updateTabTitle(int index);

    void openFileAtPath(const QString &fileName);
    bool loadFromFile(const QString &fileName);

    bool saveToFile(const QString &fileName, CodeEditor *editor);

    void autoSaveAll();

private slots:
    void newFile();

    void openFile();

    void saveFile();

    void saveFileAs();

    void closeCurrentTab();

    void selectFont();

    void showFindDialog();

    void findNext();

    void findPrevious();

    void onTabCloseRequest(int index);

    void onTabChanged(int index);

public slots:
    void bringToFront();
};

#endif
