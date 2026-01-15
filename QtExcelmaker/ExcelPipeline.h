#pragma once

#include "Excellnterfaces.h"
#include "ProcessConfig.h"
#include <functional>
#include <string>
#include <vector>

namespace core {

	// 处理阶段枚举：标识当前的处理进度阶段
	enum class Stage {
		Scanning,   // 扫描文件
		Reading,    // 读取文件
		Grouping,   // 分组/归类
		Writing,    // 写入输出
		Done        // 完成
	};

	// 进度事件结构体：用于进度汇报回调
	struct ProgressEvent {
		Stage stage = Stage::Scanning;  // 当前进度阶段
		std::string message;            // 进度说明消息（可选）
		std::size_t current = 0;        // 当前进度数
		std::size_t total = 0;          // 总进度数
	};

	// 进度回调类型定义
	using ProgressSink = std::function<void(const ProgressEvent&)>;

	// Pipeline运行结果数据
	struct Result {
		std::size_t filesScanned = 0;          // 扫描到的文件数
		std::size_t filesProcessed = 0;        // 成功处理的文件数
		std::size_t routesWritten = 0;         // 写入的“线路”数量
		std::size_t rowsWritten = 0;           // 写入的总行数
		std::vector<std::string> warnings;     // 警告信息
		std::vector<std::string> errors;       // 错误信息

		// 判断结果是否无错误
		bool ok() const { return errors.empty(); }
	};

	// Excel 数据处理流水线
	class ExcelPipeline final {
	public:
		// 构造函数，需传入Excel读取器、写入器和可选进度回调
		ExcelPipeline(IExcelReader& reader, IExcelWriter& writer, ProgressSink progress = {});

		// 主运行接口，根据配置执行一次流程
		Result run(const ProcessConfig& config);

	private:
		// 用于触发/报告进度事件
		void reportProgress(Stage stage, std::string message, std::size_t current = 0, std::size_t total = 0);
		
		IExcelReader& reader_;      // Excel读取器依赖
		IExcelWriter& writer_;      // Excel写入器依赖
		ProgressSink progress_;     // 进度回调
	};

}  // namespace core
