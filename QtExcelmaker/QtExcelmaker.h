#pragma once

#include <QtWidgets/QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include "DesignSystem.h"
#include "Win11CheckButton.h"
#include "StyleSheet.h"

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

    QLineEdit* m_mixedFolderEdit;
    QLineEdit* m_roadFolderEdit;
    QLineEdit* m_facilityFolderEdit;
    QLineEdit* m_roadTemplateEdit;
    QLineEdit* m_facilityTemplateEdit;
    QLineEdit* m_outputDirEdit;
    Win11CheckButton* m_checkBox;
    QPushButton* m_startBtn;

    // 主题相关
    DesignSystem* m_designSystem;
};

