#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace core {

	// 输出类型：路基或设施
	enum class OutputKind {
		Road,      // 路基
		Facility   // 设施
	};

	// 过程配置项结构体
	struct ProcessConfig final {
		// 文件路径相关
		std::filesystem::path mixedFolder;        // 混合文件夹路径
		std::filesystem::path roadFolder;         // 路基文件夹路径
		std::filesystem::path facilityFolder;     // 设施文件夹路径

		// 模板文件路径
		std::filesystem::path roadTemplate;       // 路基模板文件路径
		std::filesystem::path facilityTemplate;   // 设施模板文件路径
		std::filesystem::path outputDir;          // 输出目录路径

		// 分类用关键词
		std::vector<std::string> roadKeywords;      // 路基关键词
		std::vector<std::string> facilityKeywords;  // 设施关键词

		bool includeSubfolders = true;              // 是否包含子文件夹（默认包含）
		std::vector<std::string> extensions = { ".xlsx" }; // 允许的文件扩展名

		// 列头及工作表相关
		std::string routeNameHeader = "线路名称"; 
		std::optional<std::string> inputSheetName;    // 输入sheet名（可选）
		std::optional<std::string> outputSheetName;   // 输出sheet名（可选）

		std::size_t headerRowIndex = 1;       // 列头所在行索引（1为首行）
		std::size_t dataStartRowIndex = 2;    // 数据起始行索引

		std::size_t outputStartRowIndex = 2;  // 输出表起始行索引
		std::size_t outputStartColIndex = 1;  // 输出表起始列索引

		bool splitOutputByKind = true;        // 是否按类型分输出（默认为true）
		std::string roadFolderName = "road";        // 路基输出文件夹名称
		std::string facilityFolderName = "facility";// 设施输出文件夹名称
		std::string outputExtension = ".xlsx";      // 输出文件扩展名
		bool overwriteExisting = true;              // 输出时是否覆盖已存在文件
	};

}  // namespace core
