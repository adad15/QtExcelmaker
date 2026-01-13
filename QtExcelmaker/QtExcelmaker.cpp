#include "QtExcelmaker.h"
#include <QDebug>

QtExcelmaker::QtExcelmaker(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

QtExcelmaker::~QtExcelmaker()
{}

void QtExcelmaker::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    /*
    ┌─────────────────────────────────┐
    │                            20px (上)                             │
    │   ┌────────────────────────────┐   │
    │30 │                                                        │30 │
    │px │      实际内容区域                                      │px │
    │   │                                                        │   │
    │   └────────────────────────────┘   │
    │                             20px (下)                            │
    └─────────────────────────────────┘
    */
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(15);

    // === 数据源区域 ===
    auto* dataSourceLabel = new QLabel("数据源", this);
    dataSourceLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    mainLayout->addWidget(dataSourceLabel);

    // 混合文件夹
    mainLayout->addLayout(createFileRow("混合文件夹", &m_mixedFolderEdit));
    // 路基文件夹
    mainLayout->addLayout(createFileRow("路基文件夹", &m_roadFolderEdit));
    // 设施文件夹
	mainLayout->addLayout(createFileRow("设施文件夹", &m_facilityFolderEdit));

    mainLayout->addSpacing(10);

    // === 输出与配置区域 ===
    auto* outputLabel = new QLabel("输出与配置", this);
    outputLabel->setStyleSheet("font-weight: bold; font-size: 14px;");

    mainLayout->addWidget(outputLabel);
    // 路基模板文件
    mainLayout->addLayout(createFileRow("路基模板文件", &m_roadTemplateEdit));
    // 设施模板文件
    mainLayout->addLayout(createFileRow("设施模板文件", &m_facilityTemplateEdit));
    // 输出目录
    mainLayout->addLayout(createFileRow("输出目录", &m_outputDirEdit));
 
    // === 复选框区域 ===
    auto* checkLayout = new QHBoxLayout();
    m_checkBox = new Win11CheckButton(this);
    m_checkBox->setChecked(true);
    auto* checkLabel = new QLabel(
        "包含子文件夹          按 B 列（路线名称）汇总：混合文件夹按文件名关键词区分。", this);
    checkLayout->addWidget(m_checkBox);
    checkLayout->addWidget(checkLabel);
    checkLayout->addStretch();
    mainLayout->addLayout(checkLayout);

    mainLayout->addSpacing(20);

    // === 开始处理按钮 ===
    m_startBtn = new QPushButton("开始处理", this);
    m_startBtn->setFixedHeight(45);
    m_startBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #1677FF;
                color: white;
                border: none;
                border-radius: 6px;
                font-size: 15px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #4096FF;
            }
            QPushButton:pressed {
                background-color: #0958D9;
            }
        )");
    mainLayout->addWidget(m_startBtn);

    mainLayout->addStretch();
    connect(m_startBtn, &QPushButton::clicked, [this]() {
        qDebug() << "混合文件夹:" << m_mixedFolderEdit->text();
        qDebug() << "路基文件夹:" << m_roadFolderEdit->text();
        qDebug() << "设施文件夹:" << m_facilityFolderEdit->text();
        });
    // === 底部提示 ===
    auto* tipLabel = new QLabel("ⓘ 请选择数据源和模板，然后点击开始处理。", this);
    tipLabel->setStyleSheet("color: #888888; font-size: 12px;");
    mainLayout->addWidget(tipLabel);
}

QHBoxLayout* QtExcelmaker::createFileRow(const QString& labelText, QLineEdit** lineEdit)
{
    auto* layout = new QHBoxLayout();

    auto* label = new QLabel(labelText, this);
    label->setFixedWidth(100);

    *lineEdit = new QLineEdit(this);
    (*lineEdit)->setFixedHeight(36);
    (*lineEdit)->setStyleSheet(R"(
            QLineEdit {
                border: 1px solid #D9D9D9;
                border-radius: 6px;
                padding: 0 10px;
                background: white;
            }
            QLineEdit:focus {
                border: 1px solid #1677FF;
            }
        )");
    auto* folderBtn = new QPushButton(this);
    folderBtn->setFixedSize(36, 36);
    folderBtn->setIcon(QIcon(":/QtExcelmaker/icons/folder.png")); // 需要文件夹图标
    folderBtn->setStyleSheet("border: 1px solid #D9D9D9; border-radius: 6px;");

    // 连接选择按钮
    connect(folderBtn, &QPushButton::clicked, [lineEdit]() {
        QString dir = QFileDialog::getExistingDirectory(nullptr, "选择文件夹");
        if (!dir.isEmpty()) {
            (*lineEdit)->setText(dir);
        }
        });

    layout->addWidget(label);
    layout->addWidget(*lineEdit, 1);
    layout->addWidget(folderBtn);

    return layout;
}

