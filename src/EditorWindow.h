#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QFont>

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

    QSettings settings {"kijitabu", "kijitabu"};

    void createMenus();

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
