#pragma once

#include <QtWidgets/QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <vector>
#include "DesignSystem.h"
#include "Win11CheckButton.h"
#include "StyleSheet.h"

class QDialog;
class QProgressBar;
class QTextEdit;
class QResizeEvent;
class QEvent;

// 继承自 QMainWindow（Qt 主窗口类）
//class QtExcelmaker : public QMainWindow
class QtExcelmaker : public QWidget
{
    Q_OBJECT

public:
    QtExcelmaker(QWidget *parent = nullptr);
    ~QtExcelmaker();

private:
    void setupUI();
    QWidget* creatCard(const QString& title, QLayout* const contentLayout);
    QHBoxLayout* createFileRow(const QString& labelText, QLineEdit** lineEdit, bool isFolder = true);
    void applyThemeStyles();
    void ensureProgressDialog();
    void appendProgressLog(const QString& text);
    void setProgressRunning();
    void setProgressFinished(bool ok, const QString& summary);
    void setCardShadowsEnabled(bool enabled);
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    QLineEdit* m_mixedFolderEdit;
    QLineEdit* m_roadFolderEdit;
    QLineEdit* m_facilityFolderEdit;
    QLineEdit* m_roadTemplateEdit;
    QLineEdit* m_facilityTemplateEdit;
    QLineEdit* m_outputDirEdit;
    Win11CheckButton* m_checkBox;
    QPushButton* m_startBtn;
    QDialog* m_progressDialog = nullptr;
    QTextEdit* m_progressLog = nullptr;
    QLabel* m_progressStatus = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_progressCloseBtn = nullptr;
    QTimer* m_resizeDebounce = nullptr;
    QTimer* m_progressResizeDebounce = nullptr;
    std::vector<QGraphicsDropShadowEffect*> m_cardShadows;

    // 主题相关
    DesignSystem* m_designSystem;
};

