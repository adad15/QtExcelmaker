#include "QtExcelmaker.h"
#include <QDebug>

QtExcelmaker::QtExcelmaker(QWidget *parent)
    : QWidget(parent)
	, m_designSystem(DesignSystem::instance())
{
    setupUI();
    applyThemeStyles();
}

QtExcelmaker::~QtExcelmaker()
{}

void QtExcelmaker::setupUI()
{
    const Theme& theme = m_designSystem->currentTheme();

    auto* mainLayout = new QVBoxLayout(this);
    /*
    ┌─────────────────────────────────┐
    │                            24px (上)                             │
    │   ┌────────────────────────────┐   │
    │30 │                                                        │30 │
    │px │      实际内容区域                                      │px │
    │   │                                                        │   │
    │   └────────────────────────────┘   │
    │                             24px (下)                            │
    └─────────────────────────────────┘
    */
    mainLayout->setContentsMargins(30, 24, 30, 24);
    mainLayout->setSpacing(20);

    // === 数据源卡片 ===
    auto* dataSourceLayout = new QVBoxLayout();
    dataSourceLayout->setSpacing(10);
    dataSourceLayout->addLayout(createFileRow("混合文件夹", &m_mixedFolderEdit));
    dataSourceLayout->addLayout(createFileRow("路基文件夹", &m_roadFolderEdit));
    dataSourceLayout->addLayout(createFileRow("设施文件夹", &m_facilityFolderEdit));

    mainLayout->addWidget(creatCard("数据源", dataSourceLayout));
 
    // === 输出与配置卡片 ===
    auto* outputLayout = new QVBoxLayout();
    outputLayout->setSpacing(10);
    outputLayout->addLayout(createFileRow("路基模板文件", &m_roadTemplateEdit, false));
    outputLayout->addLayout(createFileRow("设施模板文件", &m_facilityTemplateEdit, false));
    outputLayout->addLayout(createFileRow("输出目录", &m_outputDirEdit));

    mainLayout->addWidget(creatCard("输出与配置", outputLayout));

    // === 复选框区域 ===
    auto* checkLayout = new QHBoxLayout();
    checkLayout->setSpacing(0);

    m_checkBox = new Win11CheckButton(this);
    m_checkBox->setChecked(true);
    // Win11CheckButton 的高度太小，绘制的圆角框和对勾被上下裁掉了一点
    m_checkBox->setFixedHeight(24);

    auto* checkLabel = new QLabel("包含子文件夹", this);
    checkLabel->setStyleSheet(QString("color: %1; font-size: 13px;").arg(theme.textColor.name()));

    auto* descLabel = new QLabel("按 B 列（路线名称）汇总：混合文件夹按文件名关键词区分", this);
    descLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(theme.disabledColor.name()));

    checkLayout->addWidget(m_checkBox);
    checkLayout->addWidget(checkLabel);
    checkLayout->addSpacing(20);
	checkLayout->addWidget(descLabel);
    checkLayout->addStretch();
    mainLayout->addLayout(checkLayout);


    // === 开始处理按钮 ===
    m_startBtn = new QPushButton("开始处理", this);
    m_startBtn->setFixedHeight(45);
    m_startBtn->setCursor(Qt::PointingHandCursor);
    m_startBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: %2;
        }
        QPushButton:pressed {
            background-color: %3;
        }
    )").arg(theme.primaryColor.name(),theme.primaryHoverColor.name(),theme.primaryColor.darker(110).name()));
    mainLayout->addWidget(m_startBtn);

    mainLayout->addStretch();

    // === 底部提示 ===
    auto* tipLabel = new QLabel("ⓘ 请选择数据源和模板，然后点击开始处理。", this);
    tipLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(theme.disabledColor.name()));
    mainLayout->addWidget(tipLabel);

    // 连接信号
    connect(m_startBtn, &QPushButton::clicked, [this]() {
        qDebug() << "混合文件夹:" << m_mixedFolderEdit->text();
        qDebug() << "路基文件夹:" << m_roadFolderEdit->text();
        qDebug() << "设施文件夹:" << m_facilityFolderEdit->text();
        });
    
}

QWidget* QtExcelmaker::creatCard(const QString& title, QLayout* const contentLayout)
{
    const Theme& theme = m_designSystem->currentTheme();

	auto* card = new QWidget(this);
    card->setObjectName("AntCard");
    card->setStyleSheet(QString(R"(
        #AntCard {
            background-color: %1;
            border-radius: 8px;
            border: 1px solid %2;
        }
    )").arg(theme.widgetBgColor.name(),theme.borderColor.name()));

    // 添加阴影效果
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(12);
    shadow->setColor(theme.shadowColor);
    shadow->setOffset(0, 2);
    card->setGraphicsEffect(shadow);

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 16, 20, 16);
    cardLayout->setSpacing(12);

    // 标题
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet(QString(R"(
        font-weight: 600;
        font-size: 15px;
        color: %1;
        padding-top: 6px;
        padding-bottom: 8px;
        border-bottom: 1px solid %2;
    )").arg(theme.textColor.name(), theme.borderColor.name()));
    cardLayout->addWidget(titleLabel);

    // 内容区域
    auto* contentWidget = new QWidget(card);
    contentWidget->setLayout(contentLayout);
    cardLayout->addWidget(contentWidget);

    return card;
}

QHBoxLayout* QtExcelmaker::createFileRow(const QString& labelText, QLineEdit** lineEdit, bool isFolder)
{
    const auto& theme = m_designSystem->currentTheme();

    auto* layout = new QHBoxLayout();
    layout->setSpacing(12);

    auto* label = new QLabel(labelText, this);
    label->setFixedWidth(90);
    label->setStyleSheet(QString("color: %1; font-size: 13px;").arg(theme.textColor.name()));

    *lineEdit = new QLineEdit(this);
    (*lineEdit)->setFixedHeight(36);
    (*lineEdit)->setObjectName("AntBaseInput");
    (*lineEdit)->setPlaceholderText(isFolder ? "请选择文件夹..." : "请选择文件...");
    (*lineEdit)->setStyleSheet(StyleSheet::antBaseInputQss(
        theme.lineEditBorderColor,
        theme.widgetBgColor,
        theme.widgetHoverBgColor,
        theme.primaryColor,
        theme.widgetBgColor,
        theme.textColor,
        theme.placeholderColor,
        8
    ));

    auto* folderBtn = new QPushButton(this);
    folderBtn->setFixedSize(36, 36);
    folderBtn->setCursor(Qt::PointingHandCursor);
    folderBtn->setIcon(QIcon(":/QtExcelmaker/icons/folder.png")); // 需要文件夹图标
    folderBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: %3;
            border-color: %4;
        }
    )").arg(theme.widgetBgColor.name(),theme.borderColor.name(),theme.widgetHoverBgColor.name(),theme.primaryColor.name()));

    // 连接选择按钮
    connect(folderBtn, &QPushButton::clicked, [lineEdit, isFolder]() {
        QString path;
        if (isFolder) {
            path = QFileDialog::getExistingDirectory(nullptr, "选择文件夹");
        }
        else {
            path = QFileDialog::getOpenFileName(nullptr, "选择文件", "", "Excel Files (*.xlsx *.xls);;All Files (*)");
        }
        if (!path.isEmpty()) {
            (*lineEdit)->setText(path);
        }
        });

    layout->addWidget(label);
    layout->addWidget(*lineEdit, 1);
    layout->addWidget(folderBtn);

    return layout;
}

void QtExcelmaker::applyThemeStyles()
{
    const auto& theme = m_designSystem->currentTheme();

    // 应用背景色
    setStyleSheet(QString("background-color: %1;").arg(theme.backgroundColor.name()));
}

