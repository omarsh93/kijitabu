#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QTextEdit>
#include <QTextStream>

class TextEditor : public QMainWindow {
public:
    TextEditor() {
        textEdit = new QTextEdit(this);
        setCentralWidget(textEdit);

        createMenus();

        //setWindowTitle("Simple Qt Text Editor");
        setWindowTitle("Kijitabu");
        resize(800, 600);
    }

private:
    QTextEdit *textEdit;
    QString currentFile;

    void createMenus() {
        QMenu *fileMenu = menuBar()->addMenu("ファイル");

        QAction *newAction = fileMenu->addAction("新規");
        QAction *openAction = fileMenu->addAction("開く");
        QAction *saveAction = fileMenu->addAction("保存");
        QAction *saveAsAction = fileMenu->addAction("名前を付けて保存");
        fileMenu->addSeparator();
        QAction *exitAction = fileMenu->addAction("終了");

        connect(newAction, &QAction::triggered, this, [this]() {
            textEdit->clear();
            currentFile.clear();
            setWindowTitle("Kijitabu");
        });

        connect(openAction, &QAction::triggered, this, [this]() {
            QString fileName = QFileDialog::getOpenFileName(
                this,
                "ファイルを開く"
            );

            if (fileName.isEmpty()) {
                return;
            }

            QFile file(fileName);

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::warning(this, "エラー", "ファイルを開けませんでした");
                return;
            }

            QTextStream in(&file);
            textEdit->setPlainText(in.readAll());

            currentFile = fileName;
            setWindowTitle(currentFile);
        });

        connect(saveAction, &QAction::triggered, this, [this]() {
            saveFile();
        });

        connect(saveAsAction, &QAction::triggered, this, [this]() {
            saveFileAs();
        });

        connect(exitAction, &QAction::triggered, this, &QWidget::close);
    }

    bool saveFile() {
        if (currentFile.isEmpty()) {
            return saveFileAs();
        }

        QFile file(currentFile);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "エラー", "ファイルを保存できませんでした");
            return false;
        }

        QTextStream out(&file);
        out << textEdit->toPlainText();

        return true;
    }

    bool saveFileAs() {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "名前を付けて保存"
        );

        if (fileName.isEmpty()) {
            return false;
        }

        currentFile = fileName;
        setWindowTitle(currentFile);

        return saveFile();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    TextEditor editor;
    editor.show();

    return app.exec();
}

