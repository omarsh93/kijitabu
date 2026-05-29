#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QSettings>
#include <QFont>
#include <QStringList>

class QPlainTextEdit;

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QPlainTextEdit *textEdit;

    QString currentFile;

    QMenu *recentFilesMenu;
    QSettings settings {"kijitabu", "kijitabu"};

    QStringList recentFiles;
    static constexpr int maxRecentFiles = 10;
    static constexpr const char *untitledSentinel = "__untitled__";

    void createMenus();
    void updateRecentFilesMenu();
    void addToRecentFiles(const QString &fileName);

    void updateWindowTitle();

    void applyFont(const QFont &font);

    void restoreLastSession();

    QString autoSavePath();

    bool loadFromFile(const QString &fileName);

    bool saveToFile(const QString &fileName);

    void autoSave();

    void loadAutoSave();

private slots:
    void newFile();

    void openFile();

    void saveFile();

    void saveFileAs();

    void selectFont();

public slots:
    void bringToFront();
};

#endif
