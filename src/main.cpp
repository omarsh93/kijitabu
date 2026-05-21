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

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow()
    {
        textEdit = new QTextEdit(this);

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
    QTextEdit *textEdit;
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
    QApplication::setOrganizationName("Local");

    EditorWindow window;
    window.show();

    return app.exec();
}
