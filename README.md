# QtExcelmaker

一个基于 Qt 框架的 Excel 文件处理工具，提供现代化的用户界面，支持浅色/深色主题切换。

## 项目概述

QtExcelmaker 是一个桌面应用程序，用于处理 Excel 文件数据。主要功能是从多个数据源（混合文件夹、路基文件夹、设施文件夹）读取 Excel 文件，根据模板文件进行处理，并将结果输出到指定目录。

---

## 项目结构

```
QtExcelmaker/
├── QtExcelmaker.sln              # Visual Studio 解决方案文件
├── QtExcelmaker/
│   ├── main.cpp                  # 程序入口点
│   ├── QtExcelmaker.h/.cpp       # 主窗口类
│   ├── QtExcelmaker.ui           # Qt Designer UI 文件
│   ├── QtExcelmaker.qrc          # Qt 资源文件
│   ├── QtExcelmaker.vcxproj      # Visual Studio 项目文件
│   │
│   ├── DesignSystem.h/.cpp       # 设计系统（主题管理）
│   ├── StyleSheet.h              # 样式表工具函数
│   │
│   ├── Win11CheckButton.h/.cpp   # Windows 11 风格复选按钮
│   ├── CustomToolButton.h/.cpp   # 自定义工具按钮
│   ├── MaskWidget.h/.cpp         # 遮罩层组件
│   ├── TransparentMask.h/.cpp    # 透明遮罩层组件
│   │
│   └── icons/                    # 图标资源目录
│       └── folder.png            # 文件夹图标
```

---

## 核心组件

### 1. 主窗口（QtExcelmaker）

主窗口类继承自 `QWidget`，负责构建整个用户界面。

**主要职责：**
- 创建和管理 UI 布局
- 处理用户交互事件
- 应用主题样式

**UI 结构：**
```
┌──────────────────────────────────────────────────────────────┐
│                         主窗口                                │
├──────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ 数据源卡片                                                │ │
│  │  • 混合文件夹输入框                                       │ │
│  │  • 路基文件夹输入框                                       │ │
│  │  • 设施文件夹输入框                                       │ │
│  └─────────────────────────────────────────────────────────┘ │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ 输出与配置卡片                                            │ │
│  │  • 路基模板文件输入框                                     │ │
│  │  • 设施模板文件输入框                                     │ │
│  │  • 输出目录输入框                                         │ │
│  └─────────────────────────────────────────────────────────┘ │
│  ☑ 包含子文件夹  按 B 列（路线名称）汇总...                    │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                     开始处理按钮                          │ │
│  └─────────────────────────────────────────────────────────┘ │
│  ⓘ 请选择数据源和模板，然后点击开始处理。                      │
└──────────────────────────────────────────────────────────────┘
```

### 2. 设计系统（DesignSystem）

采用单例模式的全局主题管理系统，负责管理应用程序的视觉风格。

**主要功能：**
- 主题切换（浅色/深色模式）
- 颜色方案管理
- 图标资源管理
- 全局遮罩管理

**主题结构（Theme）：**

> 注：以下仅展示部分核心属性，完整结构包含 30+ 个颜色属性。

```cpp
struct Theme {
    // 主色调
    QColor primaryColor;             // 主题色
    QColor primaryHoverColor;        // 悬停色
    
    // 文本颜色
    QColor textColor;                // 主文本色
    QColor disabledColor;            // 禁用文本色
    QColor placeholderColor;         // 占位符文本色
    
    // 背景颜色
    QColor backgroundColor;          // 窗口背景色
    QColor widgetBgColor;            // 控件背景色
    QColor widgetHoverBgColor;       // 控件悬停背景色
    QColor widgetSelectedBgColor;    // 控件选中背景色
    
    // 边框与阴影
    QColor borderColor;              // 边框色
    QColor borderColorHover;         // 悬停边框色
    QColor shadowColor;              // 阴影色
    
    // 复选框颜色
    QColor checkBoxBgColor;          // 复选框背景色
    QColor checkBoxBorderEnableColor;// 启用边框色
    QColor checkBoxTextColor;        // 复选框文本色
    
    // 弹出层颜色
    QColor popupBgColor;             // 弹出层背景色
    QColor popupBorderColor;         // 弹出层边框色
    QColor popupTextColor;           // 弹出层文本色
    
    // 滚动区域颜色
    QColor scrollAreaHandleColor;    // 滚动条把手色
    QColor scrollAreaHoverColor;     // 滚动条悬停色
    
    // 输入框颜色
    QColor lineEditBorderColor;      // 输入框边框色
    
    // 还包含：消息框、滑块、切换按钮、工具提示、
    // 进度条、标签、表格、Tab栏等组件的颜色定义...
};
```

### 3. 样式表系统（StyleSheet）

提供一组内联函数，用于生成 Qt 样式表（QSS）字符串。

**主要样式函数：**
```cpp
StyleSheet::antBaseInputQss()    // 输入框样式
StyleSheet::antScrollAreaQss()   // 滚动区域样式
StyleSheet::standardDialogQss()  // 对话框样式
StyleSheet::noBorderBtnQss()     // 无边框按钮样式
// ... 更多样式
```

### 4. 自定义控件

#### Win11CheckButton
仿 Windows 11 风格的复选按钮，带有流畅的选中动画效果。

**特性：**
- 自定义绘制对勾动画
- 支持 Qt 属性动画
- 响应主题切换

#### CustomToolButton
自定义工具按钮，支持 SVG 图标的动态切换。

**特性：**
- 支持普通/激活状态图标切换
- 悬停效果
- SVG 渲染支持

#### MaskWidget / TransparentMask
遮罩层组件，用于模态交互。

**特性：**
- 透明度渐变动画
- 点击事件拦截
- 全屏覆盖

---

## 工作流程

### 应用程序启动流程

```
┌────────────────────────────────────────────────────────────────┐
│                           main()                               │
└────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────────┐
│                  创建 QApplication 实例                         │
│                  (初始化事件循环)                                │
└────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────────┐
│                  创建 QtExcelmaker 主窗口                       │
│                                                                │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ 构造函数初始化:                                           │ │
│  │  1. 获取 DesignSystem 单例                               │ │
│  │  2. 调用 setupUI() 创建界面                               │ │
│  │  3. 调用 applyThemeStyles() 应用主题                      │ │
│  └──────────────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────────┐
│                    显示主窗口 (800x600)                         │
└────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────────┐
│                   进入事件循环 (app.exec())                     │
│                   等待并处理用户交互                             │
└────────────────────────────────────────────────────────────────┘
```

### UI 构建流程 (setupUI)

```
setupUI()
    │
    ├─► 创建主布局 (QVBoxLayout)
    │       设置边距: 30px (左右), 24px (上下)
    │       设置间距: 20px
    │
    ├─► 创建数据源卡片
    │       │
    │       └─► createFileRow() × 3
    │              创建 [标签 + 输入框 + 选择按钮] 的行布局
    │
    ├─► 创建输出配置卡片
    │       │
    │       └─► createFileRow() × 3
    │
    ├─► 创建复选框区域
    │       │
    │       └─► Win11CheckButton + 描述文本
    │
    ├─► 创建开始处理按钮
    │       │
    │       └─► 连接点击信号 → 输出调试信息
    │
    └─► 创建底部提示标签
```

### 用户交互流程

```
用户点击文件夹选择按钮
         │
         ▼
┌─────────────────────────────────────┐
│   QFileDialog::getExistingDirectory │  (选择文件夹)
│   或                                 │
│   QFileDialog::getOpenFileName      │  (选择文件)
└─────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│   更新对应 QLineEdit 的文本内容      │
└─────────────────────────────────────┘

用户点击"开始处理"按钮
         │
         ▼
┌─────────────────────────────────────┐
│   触发 clicked 信号                  │
└─────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│   Lambda 回调函数执行                 │
│   输出已选择的路径到调试控制台         │
└─────────────────────────────────────┘
```

---

## 底层设计逻辑

### 1. 单例模式 - DesignSystem

```cpp
class DesignSystem {
public:
    static DesignSystem* instance() {
        if (!m_instance) {
            m_instance = new DesignSystem();
        }
        return m_instance;
    }
private:
    static DesignSystem* m_instance;
    DesignSystem();  // 私有构造函数
};
```

**优点：**
- 全局唯一实例，统一管理主题
- 延迟初始化，节省资源
- 便于组件访问全局设置

### 2. Qt 信号槽机制

应用程序大量使用 Qt 的信号槽机制进行组件间通信：

```cpp
// 按钮点击信号连接
connect(folderBtn, &QPushButton::clicked, [lineEdit, isFolder]() {
    QString path;
    if (isFolder) {
        path = QFileDialog::getExistingDirectory(...);
    } else {
        path = QFileDialog::getOpenFileName(...);
    }
    if (!path.isEmpty()) {
        (*lineEdit)->setText(path);
    }
});

// 主题变更信号
connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]() {
    themeColor = DesignSystem::instance()->primaryColor();
});
```

### 3. Qt 属性动画系统

Win11CheckButton 使用 Q_PROPERTY 和 QPropertyAnimation 实现平滑动画：

```cpp
class Win11CheckButton : public QCheckBox {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)
    
public:
    void setProgress(qreal p) {
        m_progress = p;
        update();  // 触发重绘
    }
    
private:
    QPropertyAnimation* m_anim;
};
```

### 4. 自定义绘制机制

通过重写 `paintEvent()` 实现完全自定义的控件外观：

```cpp
void Win11CheckButton::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // 绘制复选框背景
    p.setBrush(isChecked() ? themeColor : theme.checkBoxBgColor);
    p.drawRoundedRect(boxRect, radius, radius);
    
    // 绘制对勾（带动画进度）
    if (m_progress > 0.0) {
        QPainterPath checkPath;
        // ... 构建对勾路径
        p.drawPath(strokePathPortion(checkPath, drawLen));
    }
}
```

### 5. 样式表动态生成

使用内联函数动态生成样式表，支持主题色参数化：

```cpp
inline QString antBaseInputQss(
    const QColor& normalBorder,
    const QColor& normalBg,
    // ... 更多颜色参数
) {
    return QString(R"(
        #AntBaseInput {
            border: 1px solid %1;
            background-color: %2;
            // ...
        }
    )").arg(normalBorder.name())
       .arg(normalBg.name());
}
```

---

## 技术栈

| 技术 | 版本/说明 |
|------|----------|
| **C++** | C++17 或更高 |
| **Qt** | Qt 5.x / Qt 6.x |
| **IDE** | Visual Studio 2022 (v17) |
| **构建系统** | MSBuild / Qt VS Tools |
| **平台** | Windows x64 |

---

## 编译与运行

### 环境要求
- Visual Studio 2022
- Qt 6.x (建议 6.5+)
- Qt VS Tools 扩展

### 编译步骤
1. 使用 Visual Studio 打开 `QtExcelmaker.sln`
2. 确保 Qt 版本配置正确
3. 选择 `Release|x64` 或 `Debug|x64` 配置
4. 按 F5 编译并运行

---

## 未来扩展

当前项目框架已搭建完成，主要业务逻辑（Excel 文件处理）尚待实现：

- [ ] Excel 文件读取功能
- [ ] 数据按 B 列（路线名称）汇总
- [ ] 根据模板生成输出文件
- [ ] 子文件夹递归扫描
- [ ] 处理进度显示
- [ ] 错误处理与日志

---

## 许可证

[待补充]
