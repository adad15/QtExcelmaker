#include "QtExcelmaker.h"
#include <QDebug>
#include <QDialog>
#include <QProgressBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextOption>
#include <QEvent>
#include "QtExcelController.h"
#include "OpenXlsxAdapter.h"
#include <memory>

namespace {

    std::unique_ptr<core::IExcelReader> createReader() {
        return std::make_unique<core::OpenXlsxReader>();
    }

    std::unique_ptr<core::IExcelWriter> createWriter() {
        return std::make_unique<core::OpenXlsxWriter>();
    }

    QString stageLabel(int stage) {
        switch (stage) {
        case 0:
            return "Scanning";
        case 1:
            return "Reading";
        case 2:
            return "Grouping";
        case 3:
            return "Writing";
        case 4:
            return "Done";
        default:
            return "Working";
        }
    }

}  // namespace

QtExcelmaker::QtExcelmaker(QWidget *parent)
    : QWidget(parent)
	, m_designSystem(DesignSystem::instance())
{
    setupUI();
    applyThemeStyles();

    m_resizeDebounce = new QTimer(this);
    m_resizeDebounce->setSingleShot(true);
    connect(m_resizeDebounce, &QTimer::timeout, this, [this]() {
        setCardShadowsEnabled(true);
    });

    m_progressResizeDebounce = new QTimer(this);
    m_progressResizeDebounce->setSingleShot(true);
    connect(m_progressResizeDebounce, &QTimer::timeout, this, [this]() {
        if (m_progressLog) {
            m_progressLog->setUpdatesEnabled(true);
            if (auto* view = m_progressLog->viewport()) {
                view->update();
            }
        }
        if (m_progressBar) {
            m_progressBar->setUpdatesEnabled(true);
        }
        if (m_progressStatus) {
            m_progressStatus->setUpdatesEnabled(true);
        }
    });
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

    // 创建 controller（需要你提供具体 Excel 适配器实现）
    auto* controller = new QtExcelController(createReader(), createWriter(), this);

    connect(m_startBtn, &QPushButton::clicked, this, [this, controller]() {
        QtExcelController::UiInput input;
        input.mixedFolder = m_mixedFolderEdit->text();
        input.roadFolder = m_roadFolderEdit->text();
        input.facilityFolder = m_facilityFolderEdit->text();
        input.roadTemplate = m_roadTemplateEdit->text();
        input.facilityTemplate = m_facilityTemplateEdit->text();
        input.outputDir = m_outputDirEdit->text();
        input.includeSubfolders = m_checkBox->isChecked();
        ensureProgressDialog();
        setProgressRunning();
        m_startBtn->setEnabled(false);
        controller->run(input);
        });

    connect(controller, &QtExcelController::progressTextChanged, this, [this](const QString& text) {
        appendProgressLog(text);
        });

    connect(controller, &QtExcelController::progressUpdated, this,
        [this](int stage, int current, int total, const QString& message) {
            Q_UNUSED(message);
            ensureProgressDialog();
            if (!m_progressDialog) {
                return;
            }
            if (total > 0) {
                m_progressBar->setRange(0, total);
                m_progressBar->setValue(current);
            }
            else {
                m_progressBar->setRange(0, 0);
            }
            QString status = stageLabel(stage);
            if (total > 0) {
                status += QString(" %1/%2").arg(current).arg(total);
            }
            m_progressStatus->setText(status);
        });

    connect(controller, &QtExcelController::finished, this, [this](bool ok, const QString& summary) {
        setProgressFinished(ok, summary);
        m_startBtn->setEnabled(true);
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
    m_cardShadows.push_back(shadow);

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

void QtExcelmaker::setCardShadowsEnabled(bool enabled)
{
    for (auto* shadow : m_cardShadows) {
        if (shadow) {
            shadow->setEnabled(enabled);
        }
    }
}

void QtExcelmaker::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!m_resizeDebounce) {
        return;
    }
    setCardShadowsEnabled(false);
    m_resizeDebounce->start(120);
}

bool QtExcelmaker::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_progressDialog && event->type() == QEvent::Resize) {
        if (m_progressLog) {
            m_progressLog->setUpdatesEnabled(false);
        }
        if (m_progressBar) {
            m_progressBar->setUpdatesEnabled(false);
        }
        if (m_progressStatus) {
            m_progressStatus->setUpdatesEnabled(false);
        }
        if (m_progressResizeDebounce) {
            m_progressResizeDebounce->start(120);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void QtExcelmaker::ensureProgressDialog()
{
    if (m_progressDialog) {
        return;
    }

    const auto& theme = m_designSystem->currentTheme();

    m_progressDialog = new QDialog(this);
    m_progressDialog->setObjectName("ProgressDialog");
    m_progressDialog->setWindowTitle("Processing");
    m_progressDialog->setModal(false);
    m_progressDialog->setMinimumSize(640, 360);
    m_progressDialog->installEventFilter(this);

    m_progressDialog->setStyleSheet(QString(R"(
        #ProgressDialog {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 10px;
        }
    )").arg(theme.widgetBgColor.name(), theme.borderColor.name()));

    auto* layout = new QVBoxLayout(m_progressDialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel("Progress Log", m_progressDialog);
    titleLabel->setStyleSheet(QString("font-size: 14px; font-weight: 600; color: %1;")
        .arg(theme.textColor.name()));
    m_progressStatus = new QLabel("Idle", m_progressDialog);
    m_progressStatus->setStyleSheet(QString("font-size: 12px; color: %1;")
        .arg(theme.disabledColor.name()));

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_progressStatus);
    layout->addLayout(headerLayout);

    m_progressBar = new QProgressBar(m_progressDialog);
    m_progressBar->setTextVisible(false);
    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setStyleSheet(QString(R"(
        QProgressBar {
            background-color: %1;
            border: none;
            border-radius: 4px;
        }
        QProgressBar::chunk {
            background-color: %2;
            border-radius: 4px;
        }
    )").arg(theme.progressBarBgColor.name(), theme.primaryColor.name()));
    layout->addWidget(m_progressBar);

    m_progressLog = new QTextEdit(m_progressDialog);
    m_progressLog->setObjectName("ProgressLog");
    m_progressLog->setReadOnly(true);
    m_progressLog->document()->setMaximumBlockCount(2000);
    m_progressLog->setLineWrapMode(QTextEdit::NoWrap);
    m_progressLog->setWordWrapMode(QTextOption::NoWrap);
    m_progressLog->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_progressLog->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_progressLog->setUndoRedoEnabled(false);
    if (auto* view = m_progressLog->viewport()) {
        view->setAttribute(Qt::WA_StaticContents, true);
    }
    m_progressLog->setStyleSheet(QString(R"(
        QTextEdit#ProgressLog {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 6px;
            color: %3;
        }
    )").arg(theme.widgetBgColor.name(), theme.borderColor.name(), theme.textColor.name()));
    layout->addWidget(m_progressLog, 1);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_progressCloseBtn = new QPushButton("Close", m_progressDialog);
    m_progressCloseBtn->setFixedHeight(32);
    m_progressCloseBtn->setEnabled(false);
    m_progressCloseBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 0 16px;
            font-size: 13px;
        }
        QPushButton:disabled {
            background-color: %2;
            color: %3;
        }
    )").arg(theme.primaryColor.name(), theme.borderColor.name(), theme.disabledColor.name()));
    connect(m_progressCloseBtn, &QPushButton::clicked, m_progressDialog, &QDialog::hide);
    buttonLayout->addWidget(m_progressCloseBtn);
    layout->addLayout(buttonLayout);
}

void QtExcelmaker::appendProgressLog(const QString& text)
{
    ensureProgressDialog();
    if (!m_progressLog) {
        return;
    }

    const auto& theme = m_designSystem->currentTheme();
    QTextCharFormat format;
    if (text.startsWith("error:", Qt::CaseInsensitive)) {
        format.setForeground(QColor("#d4380d"));
    }
    else if (text.startsWith("warning:", Qt::CaseInsensitive)) {
        format.setForeground(QColor("#d4a106"));
    }
    else {
        format.setForeground(theme.textColor);
    }

    QTextCursor cursor(m_progressLog->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text + "\n", format);
    m_progressLog->setTextCursor(cursor);
    m_progressLog->ensureCursorVisible();
}

void QtExcelmaker::setProgressRunning()
{
    ensureProgressDialog();
    if (!m_progressDialog) {
        return;
    }

    const auto& theme = m_designSystem->currentTheme();
    m_progressLog->clear();
    m_progressStatus->setText("Running...");
    m_progressStatus->setStyleSheet(QString("font-size: 12px; color: %1;")
        .arg(theme.disabledColor.name()));
    m_progressBar->setRange(0, 0);
    m_progressBar->setValue(0);
    m_progressCloseBtn->setEnabled(false);
    m_progressDialog->show();
    m_progressDialog->raise();
    m_progressDialog->activateWindow();
}

void QtExcelmaker::setProgressFinished(bool ok, const QString& summary)
{
    ensureProgressDialog();
    if (!m_progressDialog) {
        return;
    }

    const auto& theme = m_designSystem->currentTheme();
    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(1);
    m_progressStatus->setText(ok ? "Completed" : "Completed with errors");
    m_progressStatus->setStyleSheet(QString("font-size: 12px; color: %1;")
        .arg(ok ? theme.textColor.name() : QString("#d4380d")));
    appendProgressLog(summary);
    m_progressCloseBtn->setEnabled(true);
}

