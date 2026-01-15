#pragma once

#include "Excellnterfaces.h"

namespace core {

    class OpenXlsxReader final : public IExcelReader {
    public:
        std::vector<std::string> listSheets(const std::filesystem::path& file) override;
        Sheet readSheet(const std::filesystem::path& file,
            const std::optional<std::string>& sheetName) override;
    };

    class OpenXlsxWriter final : public IExcelWriter {
    public:
        std::unique_ptr<IWorkbookWriter> openFromTemplate(
            const std::filesystem::path& templatePath,
            const std::filesystem::path& outputPath) override;
    };

}  // namespace core
