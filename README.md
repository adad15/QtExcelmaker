# QtExcelmaker

一个基于 Qt 框架开发的 Excel 数据处理工具，用于处理和汇总路基、设施等相关数据文件。

## ✨ 功能特性

- 📁 **多文件夹数据源管理**：支持混合文件夹、路基文件夹、设施文件夹的批量处理
- 📄 **模板化输出**：支持路基模板文件和设施模板文件配置
- 🎨 **现代化 UI 设计**：采用 Windows 11 风格的界面设计
- 🌓 **主题切换**：支持深色/浅色主题切换
- 🔧 **灵活配置**：支持包含子文件夹、按路线名称汇总等选项

## 🖥️ 系统要求

- Windows 10/11
- Qt 5.x 或 Qt 6.x
- Visual Studio 2019 或更高版本（用于编译）

## 📦 项目结构

```
QtExcelmaker/
├── QtExcelmaker.sln              # Visual Studio 解决方案文件
└── QtExcelmaker/
    ├── main.cpp                  # 程序入口
    ├── QtExcelmaker.cpp          # 主窗口实现
    ├── QtExcelmaker.h            # 主窗口头文件
    ├── QtExcelmaker.ui           # Qt Designer UI 文件
    ├── DesignSystem.cpp          # 设计系统（主题、颜色管理）
    ├── DesignSystem.h            # 设计系统头文件
    ├── StyleSheet.h              # 样式表定义
    ├── Win11CheckButton.cpp      # Windows 11 风格复选按钮
    ├── Win11CheckButton.h        # 复选按钮头文件
    ├── MaskWidget.cpp            # 遮罩组件
    ├── MaskWidget. h              # 遮罩组件头文件
    ├── TransparentMask.cpp       # 透明遮罩组件
    ├── TransparentMask.h         # 透明遮罩头文件
    ├── QtExcelmaker.qrc          # Qt 资源文件
    └── icons/                    # 图标资源目录
```

## 🚀 快速开始

### 编译方法

1. 克隆本仓库：
   ```bash
   git clone https://github.com/adad15/QtExcelmaker.git
   ```

2. 使用 Visual Studio 打开 `QtExcelmaker.sln` 解决方案文件

3. 确保已配置 Qt 环境（Qt VS Tools 插件）

4. 编译并运行项目

### 使用说明

1. **配置数据源**
   - 混合文件夹：包含混合数据的文件夹路径
   - 路基文件夹：路基相关数据文件夹路径
   - 设施文件夹：设施相关数据文件夹路径

2. **配置输出与模板**
   - 路基模板文件：用于生成路基报表的模板
   - 设施模板文件：用于生成设施报表的模板
   - 输出目录：处理结果的输出路径

3. **设置选项**
   - 勾选"包含子文件夹"可递归处理子目录
   - 支持按 B 列（路线名称）进行数据汇总

4. 点击"开始处理"按钮执行数据处理

## 🎨 主题系统

本项目实现了完整的设计系统（DesignSystem），支持：

- 浅色/深色主题切换
- 统一的颜色管理
- 自定义 UI 组件（Win11 风格复选框等）
- 响应式布局

## 📝 技术栈

- **框架**: Qt 5.x / Qt 6.x
- **语言**: C++
- **IDE**: Visual Studio 2019+
- **构建系统**: MSBuild / qmake

## 📄 许可证

本项目采用 MIT 许可证，详情请参阅 [LICENSE](LICENSE) 文件。

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建您的功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交您的更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开一个 Pull Request

## 📧 联系方式

如有问题或建议，请通过 GitHub Issues 联系。
