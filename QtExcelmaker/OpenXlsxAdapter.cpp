#include "OpenXlsxAdapter.h"

#include <OpenXLSX.hpp>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace core {
    namespace {

        std::string toUtf8Path(const std::filesystem::path& path) {
            return u8ToString(path.u8string());
        }

#ifdef _WIN32
        std::string toAnsiPath(const std::filesystem::path& path) {
            const std::wstring wide = path.wstring();
            if (wide.empty()) {
                return {};
            }
            int size = WideCharToMultiByte(CP_ACP, 0, wide.data(),
                static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
            if (size <= 0) {
                return {};
            }
            std::string result(static_cast<std::size_t>(size), '\0');
            WideCharToMultiByte(CP_ACP, 0, wide.data(),
                static_cast<int>(wide.size()), result.data(), size, nullptr, nullptr);
            return result;
        }

        std::wstring addLongPathPrefix(const std::filesystem::path& path) {
            std::filesystem::path normalized = path;
            normalized.make_preferred();
            std::wstring raw = normalized.wstring();
            if (raw.rfind(L"\\\\?\\", 0) == 0) {
                return raw;
            }
            if (raw.rfind(L"\\\\", 0) == 0) {
                return L"\\\\?\\UNC\\" + raw.substr(2);
            }
            return L"\\\\?\\" + raw;
        }

        bool isLongPath(const std::filesystem::path& path) {
            return path.wstring().size() >= 240;
        }

        std::filesystem::path shortPathOrEmpty(const std::filesystem::path& path) {
            const std::wstring longPath = addLongPathPrefix(path);
            DWORD size = GetShortPathNameW(longPath.c_str(), nullptr, 0);
            if (size == 0) {
                return {};
            }
            std::wstring buffer(static_cast<std::size_t>(size), L'\0');
            DWORD written = GetShortPathNameW(longPath.c_str(), buffer.data(), size);
            if (written == 0 || written >= size) {
                return {};
            }
            buffer.resize(written);
            return std::filesystem::path(buffer);
        }

        std::filesystem::path makeTempPath() {
            wchar_t tempDir[MAX_PATH + 1] = {};
            DWORD len = GetTempPathW(MAX_PATH, tempDir);
            if (len == 0 || len > MAX_PATH) {
                return {};
            }
            wchar_t tempFile[MAX_PATH + 1] = {};
            if (GetTempFileNameW(tempDir, L"qtx", 0, tempFile) == 0) {
                return {};
            }
            return std::filesystem::path(tempFile);
        }

        void ensureDirectoriesWin(const std::filesystem::path& dir) {
            if (dir.empty()) {
                throw std::runtime_error("output directory is empty");
            }
            std::filesystem::path current = dir.root_path();
            const std::filesystem::path relative = dir.relative_path();
            for (const auto& part : relative) {
                current /= part;
                const std::wstring currentPath = addLongPathPrefix(current);
                if (CreateDirectoryW(currentPath.c_str(), nullptr) == 0) {
                    DWORD err = GetLastError();
                    if (err == ERROR_ALREADY_EXISTS) {
                        continue;
                    }
                    DWORD attrs = GetFileAttributesW(currentPath.c_str());
                    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        continue;
                    }
                    throw std::system_error(static_cast<int>(err), std::system_category(),
                        "create directory failed");
                }
            }
        }

        void copyFileWin(const std::filesystem::path& source,
            const std::filesystem::path& target,
            bool overwrite) {
            const std::wstring srcPath = addLongPathPrefix(source);
            const std::wstring dstPath = addLongPathPrefix(target);
            if (!CopyFileW(srcPath.c_str(), dstPath.c_str(), overwrite ? FALSE : TRUE)) {
                DWORD err = GetLastError();
                throw std::system_error(static_cast<int>(err), std::system_category(), "copy file failed");
            }
        }

        void moveFileWin(const std::filesystem::path& source,
            const std::filesystem::path& target) {
            const std::wstring srcPath = addLongPathPrefix(source);
            const std::wstring dstPath = addLongPathPrefix(target);
            if (!MoveFileExW(srcPath.c_str(), dstPath.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
                DWORD err = GetLastError();
                throw std::system_error(static_cast<int>(err), std::system_category(), "move file failed");
            }
        }

        void deleteFileWin(const std::filesystem::path& target) {
            const std::wstring dstPath = addLongPathPrefix(target);
            if (!DeleteFileW(dstPath.c_str())) {
                DWORD err = GetLastError();
                if (err != ERROR_FILE_NOT_FOUND) {
                    throw std::system_error(static_cast<int>(err), std::system_category(), "delete file failed");
                }
            }
        }

        bool pathExistsWin(const std::filesystem::path& path, std::error_code& ec) {
            ec.clear();
            if (path.empty()) {
                ec = std::make_error_code(std::errc::invalid_argument);
                return false;
            }
            const std::wstring longPath = addLongPathPrefix(path);
            DWORD attrs = GetFileAttributesW(longPath.c_str());
            if (attrs == INVALID_FILE_ATTRIBUTES) {
                DWORD err = GetLastError();
                if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
                    return false;
                }
                ec = std::error_code(static_cast<int>(err), std::system_category());
                return false;
            }
            return true;
        }
#endif

        void openDocument(OpenXLSX::XLDocument& doc, const std::filesystem::path& path) {
            const std::string utf8 = toUtf8Path(path);
            try {
                doc.open(utf8);
                return;
            }
            catch (const std::exception& first) {
#ifdef _WIN32
                const std::string ansi = toAnsiPath(path);
                if (!ansi.empty() && ansi != utf8) {
                    try {
                        doc.open(ansi);
                        return;
                    }
                    catch (const std::exception& second) {
                        throw std::runtime_error(std::string("open failed (utf8/ansi): ") + first.what()
                            + " / " + second.what());
                    }
                }
#endif
                throw;
            }
        }

        OpenXLSX::XLWorksheet selectWorksheet(OpenXLSX::XLWorkbook& workbook,
            const std::optional<std::string>& sheetName) {
            auto names = workbook.worksheetNames();
            if (names.empty()) {
                throw std::runtime_error("workbook has no worksheets");
            }

            if (sheetName && !sheetName->empty()) {
                for (const auto& name : names) {
                    if (name == *sheetName) {
                        return workbook.worksheet(name);
                    }
                }
                throw std::runtime_error("sheet not found: " + *sheetName);
            }

            return workbook.worksheet(names.front());
        }

        CellValue toCellValue(const OpenXLSX::XLCellValue& value) {
            using OpenXLSX::XLValueType;
            switch (value.type()) {
            case XLValueType::Empty:
                return std::monostate{};
            case XLValueType::Boolean:
                return value.get<bool>();
            case XLValueType::Integer:
                return static_cast<double>(value.get<int64_t>());
            case XLValueType::Float:
                return value.get<double>();
            case XLValueType::String:
                return value.get<std::string>();
            case XLValueType::Error:
                return value.get<std::string>();
            default:
                return std::string();
            }
        }

        bool isEmptyValue(const OpenXLSX::XLCellValue& value) {
            using OpenXLSX::XLValueType;
            switch (value.type()) {
            case XLValueType::Empty:
                return true;
            case XLValueType::String:
                return value.get<std::string>().empty();
            default:
                return false;
            }
        }

        void assignCell(OpenXLSX::XLCell& cell, const CellValue& value) {
            std::visit([&](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    cell.value() = std::string();
                }
                else {
                    cell.value() = v;
                }
                }, value);
        }

        struct SafeInputPath {
            std::filesystem::path openPath;
            std::filesystem::path tempPath;
            bool isTemp = false;
        };

        struct SafeOutputPath {
            std::filesystem::path finalPath;
            std::filesystem::path openPath;
            std::filesystem::path tempPath;
            bool needsMove = false;
        };

        SafeInputPath prepareInputPath(const std::filesystem::path& path) {
            SafeInputPath safe{ path, {}, false };
#ifdef _WIN32
            if (!isLongPath(path)) {
                return safe;
            }
            const auto shortPath = shortPathOrEmpty(path);
            if (!shortPath.empty()) {
                safe.openPath = shortPath;
                return safe;
            }
            const auto tempPath = makeTempPath();
            if (!tempPath.empty()) {
                copyFileWin(path, tempPath, true);
                safe.openPath = tempPath;
                safe.tempPath = tempPath;
                safe.isTemp = true;
            }
#endif
            return safe;
        }

        SafeOutputPath prepareOutputPath(const std::filesystem::path& templatePath,
            const std::filesystem::path& outputPath) {
            SafeOutputPath safe{ outputPath, outputPath, {}, false };
#ifdef _WIN32
            ensureDirectoriesWin(outputPath.parent_path());
            copyFileWin(templatePath, outputPath, true);
            if (!isLongPath(outputPath)) {
                return safe;
            }
            const auto shortPath = shortPathOrEmpty(outputPath);
            if (!shortPath.empty()) {
                safe.openPath = shortPath;
                return safe;
            }
            const auto tempPath = makeTempPath();
            if (!tempPath.empty()) {
                copyFileWin(templatePath, tempPath, true);
                safe.openPath = tempPath;
                safe.tempPath = tempPath;
                safe.needsMove = true;
            }
#endif
            return safe;
        }

        class OpenXlsxWorkbookWriter final : public IWorkbookWriter {
        public:
            OpenXlsxWorkbookWriter(std::filesystem::path templatePath,
                std::filesystem::path outputPath)
                : outputPath_(std::move(outputPath))
                , open_(false) {
                std::error_code ec;
                if (outputPath_.empty()) {
                    throw std::runtime_error("output path is empty");
                }

                const bool templateExists =
#ifdef _WIN32
                    pathExistsWin(templatePath, ec);
#else
                    std::filesystem::exists(templatePath, ec);
#endif
                if (!templateExists || ec) {
                    throw std::runtime_error("template not found: " + toUtf8Path(templatePath));
                }

                safeOutput_ = prepareOutputPath(templatePath, outputPath_);
                openDocument(doc_, safeOutput_.openPath);
                open_ = true;
            }

            ~OpenXlsxWorkbookWriter() override {
                if (open_) {
                    try {
                        doc_.close();
                    }
                    catch (...) {
                        // Best effort close; never throw from destructor.
                    }
                    open_ = false;
                }
            }

            void writeRows(const std::optional<std::string>& sheetName,
                std::size_t startRow,
                std::size_t startCol,
                const std::vector<Row>& rows) override {
                auto workbook = doc_.workbook();
                auto sheet = selectWorksheet(workbook, sheetName);

                const std::size_t baseRow = startRow == 0 ? 1 : startRow;
                const std::size_t baseCol = startCol == 0 ? 1 : startCol;

                std::size_t maxCols = 0;
                for (const auto& row : rows) {
                    if (row.size() > maxCols) {
                        maxCols = row.size();
                    }
                }
                logMessage("writer: writeRows start rows=" + std::to_string(rows.size()) +
                    " maxCols=" + std::to_string(maxCols) +
                    " startRow=" + std::to_string(baseRow) +
                    " startCol=" + std::to_string(baseCol));

                const std::size_t rowCount = rows.size();
                const std::size_t logStep = rowCount <= 50 ? 1 : std::max<std::size_t>(1, rowCount / 10);

                for (std::size_t r = 0; r < rowCount; ++r) {
                    const auto& row = rows[r];
                    if (logStep == 1 || (r % logStep == 0) || (r + 1 == rowCount)) {
                        logMessage("writer: row " + std::to_string(r + 1) + "/" +
                            std::to_string(rowCount) + " cols=" + std::to_string(row.size()));
                    }
                    for (std::size_t c = 0; c < row.size(); ++c) {
                        auto cell = sheet.cell(static_cast<uint32_t>(baseRow + r),
                            static_cast<uint16_t>(baseCol + c));
                        assignCell(cell, row[c]);
                    }
                }
                logMessage("writer: writeRows done");
            }

            void save() override {
                try {
                    doc_.save();
                    doc_.close();
                }
                catch (...) {
                    open_ = false;
                    throw;
                }
                open_ = false;
#ifdef _WIN32
                if (safeOutput_.needsMove && !safeOutput_.tempPath.empty()) {
                    moveFileWin(safeOutput_.tempPath, safeOutput_.finalPath);
                }
#endif
            }

        private:
            OpenXLSX::XLDocument doc_;
            std::filesystem::path outputPath_;
            SafeOutputPath safeOutput_{};
            bool open_ = false;
        };

    }  // namespace

    std::vector<std::string> OpenXlsxReader::listSheets(const std::filesystem::path& file) {
        OpenXLSX::XLDocument doc;
        const auto safe = prepareInputPath(file);
        openDocument(doc, safe.openPath);
        auto workbook = doc.workbook();
        auto names = workbook.worksheetNames();
        doc.close();
#ifdef _WIN32
        if (safe.isTemp) {
            deleteFileWin(safe.tempPath);
        }
#endif
        return names;
    }

    Sheet OpenXlsxReader::readSheet(const std::filesystem::path& file,
        const std::optional<std::string>& sheetName) {
        OpenXLSX::XLDocument doc;
        const auto safe = prepareInputPath(file);
        openDocument(doc, safe.openPath);
        auto workbook = doc.workbook();
        auto sheet = selectWorksheet(workbook, sheetName);

        const auto rowCount = sheet.rowCount();

        Sheet result;
        result.reserve(static_cast<std::size_t>(rowCount));

        if (rowCount == 0) {
            doc.close();
            return result;
        }

        auto rows = sheet.rows(1, rowCount);
        for (auto it = rows.begin(); it != rows.end(); ++it) {
            if (!it.rowExists()) {
                result.emplace_back();
                continue;
            }

            const auto& row = *it;
            const std::vector<OpenXLSX::XLCellValue> values = row.values();
            std::size_t lastNonEmpty = 0;
            for (std::size_t i = values.size(); i > 0; --i) {
                if (!isEmptyValue(values[i - 1])) {
                    lastNonEmpty = i;
                    break;
                }
            }

            Row rowData;
            rowData.reserve(lastNonEmpty);
            for (std::size_t i = 0; i < lastNonEmpty; ++i) {
                rowData.push_back(toCellValue(values[i]));
            }
            result.push_back(std::move(rowData));
        }

#ifdef _WIN32
        if (safe.isTemp) {
            deleteFileWin(safe.tempPath);
        }
#endif
        doc.close();
        return result;
    }

    std::unique_ptr<IWorkbookWriter> OpenXlsxWriter::openFromTemplate(
        const std::filesystem::path& templatePath,
        const std::filesystem::path& outputPath) {
        return std::make_unique<OpenXlsxWorkbookWriter>(templatePath, outputPath);
    }

}  // namespace core
