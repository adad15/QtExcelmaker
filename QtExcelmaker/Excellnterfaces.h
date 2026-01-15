#pragma once

#include "ExcelTypes.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace core {

    // IExcelReader: Excel读取接口，定义了列出现有sheet和读取指定sheet的方法
    class IExcelReader {
    public:
        virtual ~IExcelReader() = default;

        // 列出Excel文件中的所有sheet名称
        virtual std::vector<std::string> listSheets(const std::filesystem::path& file) = 0;

        // 读取指定sheet（可选sheet名称）为Sheet数据结构
        virtual Sheet readSheet(const std::filesystem::path& file,
            const std::optional<std::string>& sheetName) = 0;
    };

    // IWorkbookWriter: 工作簿写入接口，定义了写入多行和保存方法
    class IWorkbookWriter {
    public:
        virtual ~IWorkbookWriter() = default;

        // 向指定sheet（可选sheet名称）从startRow/startCol位置写入多行数据
        virtual void writeRows(const std::optional<std::string>& sheetName,
            std::size_t startRow,
            std::size_t startCol,
            const std::vector<Row>& rows) = 0;

        // 保存当前工作簿
        virtual void save() = 0;
    };

    // IExcelWriter: Excel写出接口，负责从模板生成新工作簿
    class IExcelWriter {
    public:
        virtual ~IExcelWriter() = default;

        // 基于模板文件(templatePath)和输出路径(outputPath)创建并打开工作簿写入器
        virtual std::unique_ptr<IWorkbookWriter> openFromTemplate(
            const std::filesystem::path& templatePath,
            const std::filesystem::path& outputPath) = 0;
    };

    using LogSink = std::function<void(const std::string&)>;

    inline LogSink g_logSink;

    inline void setLogSink(LogSink sink) {
        g_logSink = std::move(sink);
    }

    inline void logMessage(const std::string& message) {
        if (g_logSink) {
            g_logSink(message);
        }
    }

}  // namespace core
