# QtExcelmaker

面向初学者的技术文档：从零复现这个项目的设计、框架与工作流程。

## 1. 项目目标

QtExcelmaker 是一个桌面 Excel 批处理工具，主要解决：
- 从多个目录收集 Excel 文件
- 根据“线路名称”列把数据行按线路分组
- 将分组后的数据写入模板并输出多个新 Excel
- 支持“路基/设施”按文件名关键词区分
- 支持复合表头、合计/小计行过滤、多工作簿
- 支持进度日志与错误提示

## 2. 技术栈与依赖

- C++20
- Qt 6.10.1（Qt Widgets）
- OpenXLSX（Excel 读写）
- Visual Studio 2022 (MSVC)

### OpenXLSX 依赖说明
- Debug 版本链接 `OpenXLSXd.lib`
- Release 版本链接 `OpenXLSX.lib`
- 确保 RuntimeLibrary 与 OpenXLSX 编译方式一致（/MDd vs /MD）

## 3. 总体架构

项目分为三层：

1) **UI 层**
- `QtExcelmaker`：主窗口 UI，收集用户输入并触发执行
- 进度弹窗：显示日志、进度条、错误信息

2) **控制器层**
- `QtExcelController`：把 UI 输入转换为 `ProcessConfig`
- 启动后台任务（QtConcurrent）并将日志/进度反馈到 UI

3) **业务逻辑层（核心）**
- `ExcelPipeline`：扫描文件 → 读取数据 → 按路线分组 → 输出结果
- `ProcessConfig`：业务参数配置
- `OpenXlsxAdapter`：OpenXLSX 适配器（实现读写接口）

## 4. 核心数据结构

- `CellValue`：单元格值（空 / 数值 / 布尔 / 字符串）
- `Row`：一行数据
- `Sheet`：一个工作表
- `ProcessConfig`：业务配置（输入路径、模板、输出目录、表头位置、关键词等）

## 5. 工作流程（业务逻辑）

### 5.1 总流程

1. 扫描输入目录（混合/路基/设施）  
2. 根据文件名关键词分类为“路基”或“设施”  
3. 读取 Excel（支持多工作簿）  
4. 自动识别表头与数据起始行  
5. 跳过合计/小计行  
6. 按“线路名称”分组  
7. 使用模板输出新 Excel  

### 5.2 多工作簿处理

- 如果 `inputSheetName` 未指定，则遍历所有工作簿（worksheet）
- 任意 sheet 没有表头时只记录 warning，不中断

### 5.3 复合表头识别

复合表头常见特征：
- 表头占多行（例如第 1 行是总标题，第 2 行是“轻/中/重”）
- 真正数据开始行可能不是固定行

处理策略：
- 自动在前 20 行寻找包含“线路名称”的列
- 以该列的第一条“非表头文本”行作为数据起始行
- 用表头宽度裁剪超长行，防止出现 16384 列的异常情况

### 5.4 合计/小计行过滤

包含以下关键词的行会被跳过：
- `合计`
- `小计`
- `总计`

## 6. 关键文件与职责

- `QtExcelmaker/QtExcelmaker.h/.cpp`  
  构建 UI、进度弹窗、绑定按钮与控制器

- `QtExcelmaker/QtExcelController.h/.cpp`  
  控制层，组装配置并执行后台任务

- `QtExcelmaker/ExcelPipeline.h/.cpp`  
  业务主流程、表头识别、分组与输出

- `QtExcelmaker/ProcessConfig.h`  
  业务参数定义

- `QtExcelmaker/OpenXlsxAdapter.h/.cpp`  
  Excel 读写适配器（OpenXLSX）

- `QtExcelmaker/ExcelTypes.h`  
  基础类型与单元格转换

## 7. 从零复现步骤（新手流程）

### Step 1：创建 Qt Widgets 项目

1. 用 VS2022 新建 Qt Widgets 应用  
2. 语言标准设置为 C++20  
3. 引入 Qt Widgets 模块

### Step 2：引入 OpenXLSX

1. 将 OpenXLSX 头文件与 lib 放入项目目录  
2. 配置 IncludePath / LibraryPath  
3. Debug 链接 `OpenXLSXd.lib`，Release 链接 `OpenXLSX.lib`

### Step 3：搭建核心模块

创建文件：

- `ExcelTypes.h`：CellValue/Row/Sheet 类型
- `Excellnterfaces.h`：读写接口 + 日志回调
- `OpenXlsxAdapter.h/.cpp`：OpenXLSX 适配实现
- `ProcessConfig.h`：业务配置结构体
- `ExcelPipeline.h/.cpp`：业务核心流程

### Step 4：实现控制器

创建 `QtExcelController`：
- 负责把 UI 输入映射到 `ProcessConfig`
- 使用 `QtConcurrent::run` 执行 `ExcelPipeline::run`
- 将日志输出到 UI

### Step 5：实现 UI

主窗口 `QtExcelmaker` 中需要：
- 输入目录（混合/路基/设施）
- 模板文件（路基/设施）
- 输出目录
- 选择子文件夹的开关
- 开始按钮
- 进度弹窗（日志 + 进度条 + 错误提示）

### Step 6：连接 UI 与控制器

按下“开始处理”时：
- 构造 `QtExcelController::UiInput`
- 调用 `controller->run(input)`
- 监听 `progressTextChanged`、`progressUpdated`、`finished`

## 8. 输入文件要求（约定）

1) 必须存在“线路名称”列（可在表头任意行）  
2) 表头允许复合结构  
3) 合计/小计/总计行将被自动跳过  
4) 只支持 `.xlsx`（可自行扩展 `.xlsm`）  

## 9. 输出规则

1. 文件按线路名称生成  
2. 使用路基/设施模板  
3. 输出到指定目录，可按类型分子目录  

## 10. 常见问题与排查

**Q1: 某行写入卡住**  
可能原因：Excel 把整行当成 16384 列。  
解决方案：裁剪行宽到表头有效列数（已在 `ExcelPipeline` 实现）。

**Q2: “线路名称”找不到**  
检查表头是否包含“线路名称/路线名称”等关键字。  
如果表头在多行，确保“线路名称”那一行实际出现该标题。

**Q3: Debug 版本链接失败**  
检查 `OpenXLSXd.lib` 是否与项目的 RuntimeLibrary 设置一致。

## 11. 运行方式（UI 操作）

1. 选择混合文件夹 / 路基文件夹 / 设施文件夹  
2. 选择路基模板 / 设施模板  
3. 选择输出目录  
4. 点击“开始处理”，查看进度弹窗日志  

## 12. 可扩展点

- 新增其它关键词分类（比如“桥梁/隧道”）
- 支持更多 Excel 格式（xlsm / csv）
- 输出更多列映射规则
- 添加更多日志与性能统计

---

