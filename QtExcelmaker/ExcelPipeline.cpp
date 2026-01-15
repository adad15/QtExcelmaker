#include "ExcelPipeline.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <system_error>
#include <string>
#include <chrono>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace core {
    namespace {

        // 文件类型的枚举，分别表示道路、设施、未知和不明确
        enum class FileKind {
            Road,
            Facility,
            Unknown,
            Ambiguous
        };

        // 返回字符串小写副本
        std::string toLowerCopy(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        // 返回字符串大写副本
        std::string toUpperCopy(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            return s;
        }

        // 去除字符串首尾的空白字符
        std::string trimCopy(std::string_view s) {
            std::size_t start = 0;
            while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
                ++start;
            }
            std::size_t end = s.size();
            while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
                --end;
            }
            return std::string(s.substr(start, end - start));
        }

        // 归一化表头内容：移除空白并对ASCII字符做小写处理，便于匹配
        std::string normalizeHeader(std::string_view s) {
            std::string out;
            out.reserve(s.size());
            for (std::size_t i = 0; i < s.size(); ++i) {
                unsigned char c = static_cast<unsigned char>(s[i]);
                if (c <= 0x20) {
                    continue;
                }
                // UTF-8 full-width space (U+3000): 0xE3 0x80 0x80
                if (c == 0xE3 && i + 2 < s.size()) {
                    unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
                    unsigned char c2 = static_cast<unsigned char>(s[i + 2]);
                    if (c1 == 0x80 && c2 == 0x80) {
                        i += 2;
                        continue;
                    }
                }
                if (c < 0x80) {
                    out.push_back(static_cast<char>(std::tolower(c)));
                }
                else {
                    out.push_back(static_cast<char>(c));
                }
            }
            return out;
        }

        // 归一化线路名称（目前仅去除空白）
        std::string normalizeRouteName(std::string_view s) {
            return trimCopy(s);
        }

        // 将路径转为utf-8编码字符串
        std::string pathToUtf8(const std::filesystem::path& p) {
            return u8ToString(p.u8string());
        }

        std::filesystem::path pathFromUtf8(std::string_view value) {
#ifdef _WIN32
            if (value.empty()) {
                return {};
            }
            int size = MultiByteToWideChar(CP_UTF8, 0,
                value.data(), static_cast<int>(value.size()), nullptr, 0);
            if (size <= 0) {
                return {};
            }
            std::wstring wide(static_cast<std::size_t>(size), L'\0');
            MultiByteToWideChar(CP_UTF8, 0,
                value.data(), static_cast<int>(value.size()), wide.data(), size);
            return std::filesystem::path(wide);
#else
            return std::filesystem::path(std::string(value));
#endif
        }

        std::string formatDuration(std::chrono::steady_clock::duration elapsed) {
            using namespace std::chrono;
            const auto ms = duration_cast<milliseconds>(elapsed).count();
            if (ms < 1000) {
                return std::to_string(ms) + " ms";
            }
            const auto sec = ms / 1000;
            const auto rem = ms % 1000;
            std::string remText = std::to_string(rem);
            if (remText.size() < 3) {
                remText.insert(remText.begin(), 3 - remText.size(), '0');
            }
            return std::to_string(sec) + "." + remText + " s";
        }

        // 判断nameLower是否包含关键字keywords中的任意一个（不区分大小写）
        bool containsKeyword(const std::string& nameLower, const std::vector<std::string>& keywords) {
            for (const auto& k : keywords) {
                if (k.empty()) {
                    continue;
                }
                const std::string keyLower = toLowerCopy(k);
                if (nameLower.find(keyLower) != std::string::npos) {
                    return true;
                }
            }
            return false;
        }

        // 判断文件属于哪种类型（道路、设施、未知、不明确）
        FileKind classifyFile(const std::filesystem::path& file,
            const std::vector<std::string>& roadKeywords,
            const std::vector<std::string>& facilityKeywords) {
            const std::string nameLower = toLowerCopy(pathToUtf8(file.filename()));
            const bool isRoad = containsKeyword(nameLower, roadKeywords);
            const bool isFacility = containsKeyword(nameLower, facilityKeywords);
            if (isRoad && isFacility) {
                return FileKind::Ambiguous;  // 同时包含道路和设施关键字，不明确
            }
            if (isRoad) {
                return FileKind::Road;
            }
            if (isFacility) {
                return FileKind::Facility;
            }
            return FileKind::Unknown;
        }

        // 判断文件扩展名是否在给定列表中
        bool extensionMatches(const std::filesystem::path& file, const std::vector<std::string>& exts) {
            const std::string ext = toLowerCopy(file.extension().string());
            for (const auto& e : exts) {
                if (ext == toLowerCopy(e)) {
                    return true;
                }
            }
            return false;
        }

        // 扫描目录获取所有符合条件的文件
#ifdef _WIN32
        std::wstring toLongPath(const std::filesystem::path& path) {
            std::filesystem::path normalized = path;
            normalized.make_preferred();
            std::wstring raw = normalized.wstring();
            if (raw.rfind(L"\\\\?\\", 0) == 0) {
                return raw;
            }
            if (!normalized.is_absolute()) {
                return raw;
            }
            if (raw.rfind(L"\\\\", 0) == 0) {
                return L"\\\\?\\UNC\\" + raw.substr(2);
            }
            return L"\\\\?\\" + raw;
        }

        bool getFileAttributesWin(const std::filesystem::path& path, DWORD& attrs, std::error_code& ec) {
            ec.clear();
            if (path.empty()) {
                ec = std::make_error_code(std::errc::invalid_argument);
                return false;
            }
            const std::wstring longPath = toLongPath(path);
            attrs = GetFileAttributesW(longPath.c_str());
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

        bool ensureDirectoriesWin(const std::filesystem::path& dir, std::error_code& ec) {
            ec.clear();
            if (dir.empty()) {
                ec = std::make_error_code(std::errc::invalid_argument);
                return false;
            }
            std::filesystem::path current = dir.root_path();
            const std::filesystem::path relative = dir.relative_path();
            for (const auto& part : relative) {
                current /= part;
                const std::wstring currentPath = toLongPath(current);
                if (CreateDirectoryW(currentPath.c_str(), nullptr) == 0) {
                    DWORD err = GetLastError();
                    if (err == ERROR_ALREADY_EXISTS) {
                        continue;
                    }
                    DWORD attrs = GetFileAttributesW(currentPath.c_str());
                    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        continue;
                    }
                    ec = std::error_code(static_cast<int>(err), std::system_category());
                    return false;
                }
            }
            return true;
        }

        bool isDotName(const wchar_t* name) {
            return name[0] == L'.' && (name[1] == L'\0' || (name[1] == L'.' && name[2] == L'\0'));
        }

        void scanDirectoryWin(const std::filesystem::path& root,
            bool recursive,
            const std::vector<std::string>& exts,
            const std::unordered_set<std::filesystem::path>& excluded,
            std::unordered_set<std::filesystem::path>& seen,
            std::vector<std::filesystem::path>& out) {
            std::error_code ec;
            DWORD attrs = 0;
            if (!getFileAttributesWin(root, attrs, ec)) {
                return;
            }
            if ((attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                const auto normalized = root.lexically_normal();
                if (excluded.find(normalized) == excluded.end() && extensionMatches(normalized, exts)) {
                    if (seen.insert(normalized).second) {
                        out.push_back(normalized);
                    }
                }
                return;
            }

            std::vector<std::filesystem::path> stack;
            stack.push_back(root);
            while (!stack.empty()) {
                std::filesystem::path dir = stack.back();
                stack.pop_back();

                std::filesystem::path search = dir / L"*";
                const std::wstring searchPath = toLongPath(search);
                WIN32_FIND_DATAW data{};
                HANDLE handle = FindFirstFileW(searchPath.c_str(), &data);
                if (handle == INVALID_HANDLE_VALUE) {
                    continue;
                }

                do {
                    if (isDotName(data.cFileName)) {
                        continue;
                    }
                    std::filesystem::path entryPath = dir / data.cFileName;
                    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if (recursive && (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
                            stack.push_back(entryPath);
                        }
                        continue;
                    }

                    const auto normalized = entryPath.lexically_normal();
                    if (excluded.find(normalized) != excluded.end()) {
                        continue;
                    }
                    if (!extensionMatches(normalized, exts)) {
                        continue;
                    }
                    if (seen.insert(normalized).second) {
                        out.push_back(normalized);
                    }
                } while (FindNextFileW(handle, &data));

                FindClose(handle);
            }
        }
#endif

        void scanDirectory(const std::filesystem::path& root,
            bool recursive,
            const std::vector<std::string>& exts,
            const std::unordered_set<std::filesystem::path>& excluded,
            std::unordered_set<std::filesystem::path>& seen,
            std::vector<std::filesystem::path>& out) {
#ifdef _WIN32
            scanDirectoryWin(root, recursive, exts, excluded, seen, out);
#else
            std::error_code ec;
            if (!std::filesystem::exists(root, ec)) {
                return;
            }

            auto handleEntry = [&](const std::filesystem::directory_entry& entry) {
                std::error_code statusEc;
                if (!entry.is_regular_file(statusEc)) {
                    return;
                }
                const auto normalized = entry.path().lexically_normal();
                if (excluded.find(normalized) != excluded.end()) {
                    return;
                }
                if (!extensionMatches(normalized, exts)) {
                    return;
                }
                if (seen.insert(normalized).second) {
                    out.push_back(normalized);
                }
                };

            if (recursive) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(root, ec)) {
                    if (ec) {
                        break;
                    }
                    handleEntry(entry);
                }
            }
            else {
                for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
                    if (ec) {
                        break;
                    }
                    handleEntry(entry);
                }
            }
#endif
        }

        // 在表头行查找某个表头名称对应的列索引
        std::optional<std::size_t> findColumnIndex(const Row& headerRow, std::string_view headerName) {
            const std::string target = normalizeHeader(headerName);
            if (target.empty()) {
                return std::nullopt;
            }
            for (std::size_t i = 0; i < headerRow.size(); ++i) {
                const std::string current = normalizeHeader(cellToString(headerRow[i]));
                if (!current.empty() && current == target) {
                    return i;
                }
            }
            for (std::size_t i = 0; i < headerRow.size(); ++i) {
                const std::string current = normalizeHeader(cellToString(headerRow[i]));
                if (!current.empty() && current.find(target) != std::string::npos) {
                    return i;
                }
            }
            return std::nullopt;
        }

        void addHeaderCandidate(std::vector<std::string>& candidates, std::string value) {
            if (value.empty()) {
                return;
            }
            for (const auto& existing : candidates) {
                if (existing == value) {
                    return;
                }
            }
            candidates.push_back(std::move(value));
        }

        std::vector<std::string> buildHeaderCandidates(std::string_view headerName) {
            std::vector<std::string> candidates;
            addHeaderCandidate(candidates, trimCopy(headerName));
            addHeaderCandidate(candidates, u8ToString(u8"线路名称"));
            addHeaderCandidate(candidates, u8ToString(u8"路线名称"));
            addHeaderCandidate(candidates, u8ToString(u8"线路名"));
            addHeaderCandidate(candidates, u8ToString(u8"路线名"));
            return candidates;
        }

        std::vector<std::string> buildNormalizedHeaderCandidates(std::string_view headerName) {
            std::vector<std::string> normalized;
            for (const auto& candidate : buildHeaderCandidates(headerName)) {
                const std::string value = normalizeHeader(candidate);
                if (!value.empty()) {
                    normalized.push_back(value);
                }
            }
            return normalized;
        }

        std::optional<std::pair<std::size_t, std::size_t>> findHeaderLocation(
            const Sheet& sheet,
            std::size_t preferredRowIndex,
            std::string_view headerName) {
            if (sheet.empty()) {
                return std::nullopt;
            }

            const auto candidates = buildHeaderCandidates(headerName);

            if (preferredRowIndex > 0 && preferredRowIndex <= sheet.size()) {
                const std::size_t headerIndex = preferredRowIndex - 1;
                for (const auto& candidate : candidates) {
                    const auto colIndex = findColumnIndex(sheet[headerIndex], candidate);
                    if (colIndex) {
                        return std::make_pair(headerIndex, *colIndex);
                    }
                }
            }

            constexpr std::size_t kMaxHeaderScanRows = 20;
            const std::size_t scanRows = std::min(sheet.size(), kMaxHeaderScanRows);
            for (std::size_t rowIndex = 0; rowIndex < scanRows; ++rowIndex) {
                for (const auto& candidate : candidates) {
                    const auto colIndex = findColumnIndex(sheet[rowIndex], candidate);
                    if (colIndex) {
                        return std::make_pair(rowIndex, *colIndex);
                    }
                }
            }

            return std::nullopt;
        }

        // 判断一行是否全为空
        bool rowIsEmpty(const Row& row) {
            for (const auto& cell : row) {
                if (!trimCopy(cellToString(cell)).empty()) {
                    return false;
                }
            }
            return true;
        }

        bool rowContainsAny(const Row& row, const std::vector<std::string>& tokens) {
            for (const auto& cell : row) {
                const std::string value = trimCopy(cellToString(cell));
                if (value.empty()) {
                    continue;
                }
                for (const auto& token : tokens) {
                    if (value.find(token) != std::string::npos) {
                        return true;
                    }
                }
            }
            return false;
        }

        bool isHeaderCellValue(std::string_view value,
            const std::vector<std::string>& normalizedCandidates) {
            const std::string normalized = normalizeHeader(value);
            if (normalized.empty()) {
                return false;
            }
            for (const auto& candidate : normalizedCandidates) {
                if (normalized == candidate) {
                    return true;
                }
            }
            return false;
        }

        std::size_t detectDataStartIndex(const Sheet& sheet,
            std::size_t headerIndex,
            std::size_t colIndex,
            std::size_t scanStartIndex,
            const std::vector<std::string>& normalizedCandidates) {
            if (sheet.empty()) {
                return headerIndex + 1;
            }
            const std::size_t start = std::max(headerIndex + 1, scanStartIndex);
            const std::size_t maxScan = std::min(sheet.size(), start + 20);
            std::size_t lastHeaderRow = headerIndex;

            for (std::size_t r = start; r < maxScan; ++r) {
                const Row& row = sheet[r];
                std::string routeCell;
                if (colIndex < row.size()) {
                    routeCell = trimCopy(cellToString(row[colIndex]));
                }
                const bool hasRouteValue = !routeCell.empty();
                const bool isHeaderValue = hasRouteValue &&
                    isHeaderCellValue(routeCell, normalizedCandidates);
                if (hasRouteValue && !isHeaderValue) {
                    return r;
                }
                if (!rowIsEmpty(row)) {
                    lastHeaderRow = r;
                }
            }
            return lastHeaderRow + 1;
        }

        std::size_t lastNonEmptyColumn(const Row& row) {
            for (std::size_t i = row.size(); i > 0; --i) {
                if (!trimCopy(cellToString(row[i - 1])).empty()) {
                    return i;
                }
            }
            return 0;
        }

        // 清理并规范化文件名，防止包含无效/非法字符
        std::string sanitizeFileName(std::string name) {
            for (char& c : name) {
                const bool invalid =
                    (static_cast<unsigned char>(c) < 0x20) || c == '<' || c == '>' || c == ':' ||
                    c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*';
                if (invalid) {
                    c = '_';
                }
            }
            // Windows文件名不能以空格或点结尾
            while (!name.empty() && (name.back() == ' ' || name.back() == '.')) {
                name.pop_back();
            }
            // Windows文件名不能以空格开头
            while (!name.empty() && name.front() == ' ') {
                name.erase(name.begin());
            }
            if (name.empty()) {
                name = "route";
            }

            // Windows保留名处理
            const std::string baseUpper = toUpperCopy(name);
            static const std::unordered_set<std::string> reserved = {
                "CON", "PRN", "AUX", "NUL",
                "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
            if (reserved.find(baseUpper) != reserved.end()) {
                name = "_" + name;
            }
            return name;
        }

        // 统一处理文件扩展名（添加.前缀，默认.xlsx）
        std::string normalizeExtension(std::string ext) {
            if (ext.empty()) {
                return ".xlsx";
            }
            if (ext[0] != '.') {
                ext.insert(ext.begin(), '.');
            }
            return ext;
        }

        // 构建输出目录，含有按类型分文件夹的机制
        std::filesystem::path buildOutputDir(const ProcessConfig& config, OutputKind kind) {
            std::filesystem::path dir = config.outputDir;
            if (config.splitOutputByKind) {
                dir /= (kind == OutputKind::Road ? config.roadFolderName : config.facilityFolderName);
            }
            return dir;
        }

        // 构建单个输出文件路径（支持自动避免重名覆盖）
        bool pathExists(const std::filesystem::path& path, std::error_code& ec) {
            ec.clear();
#ifdef _WIN32
            DWORD attrs = 0;
            const bool ok = getFileAttributesWin(path, attrs, ec);
            if (!ok) {
                return false;
            }
            return true;
#else
            return std::filesystem::exists(path, ec);
#endif
        }

        bool createDirectories(const std::filesystem::path& dir, std::error_code& ec) {
            ec.clear();
#ifdef _WIN32
            return ensureDirectoriesWin(dir, ec);
#else
            std::filesystem::create_directories(dir, ec);
            return !ec;
#endif
        }

        std::filesystem::path buildOutputPath(const ProcessConfig& config,
            OutputKind kind,
            const std::string& routeName,
            std::unordered_set<std::filesystem::path>& usedPaths,
            Result& result) {
            const std::string baseName = sanitizeFileName(routeName);
            const std::string ext = normalizeExtension(config.outputExtension);

            std::filesystem::path dir = buildOutputDir(config, kind);
            std::error_code ec;
            if (!createDirectories(dir, ec)) {
                result.errors.push_back("create output dir failed: " + pathToUtf8(dir));
                return {};
            }

            const std::filesystem::path basePath = pathFromUtf8(baseName + ext);
            if (basePath.empty()) {
                result.errors.push_back("output filename conversion failed for route: " + baseName);
                return {};
            }
            std::filesystem::path candidate = (dir / basePath).lexically_normal();
            // 如果不允许覆盖已有文件，增加后缀避重命名
            if (!config.overwriteExisting) {
                int suffix = 1;
                while (pathExists(candidate, ec) || usedPaths.find(candidate) != usedPaths.end()) {
                    if (ec) {
                        result.errors.push_back("exists check failed: " + pathToUtf8(candidate));
                        return {};
                    }
                    ++suffix;
                    const std::string suffixName = baseName + "_" + std::to_string(suffix) + ext;
                    const std::filesystem::path suffixPath = pathFromUtf8(suffixName);
                    if (suffixPath.empty()) {
                        result.errors.push_back("output filename conversion failed for route: " + baseName);
                        return {};
                    }
                    candidate = (dir / suffixPath).lexically_normal();
                }
            }
            if (!candidate.empty()) {
                usedPaths.insert(candidate);
            }
            return candidate;
        }

    }  // namespace

    // ExcelPipeline 构造函数
    ExcelPipeline::ExcelPipeline(IExcelReader& reader, IExcelWriter& writer, ProgressSink progress)
        : reader_(reader), writer_(writer), progress_(std::move(progress)) {
    }

    // 进度通知
    void ExcelPipeline::reportProgress(Stage stage, std::string message, std::size_t current, std::size_t total) {
        if (progress_) {
            progress_({ stage, std::move(message), current, total });
        }
    }

    // 核心流程：读取所有文件，分组，输出新文件
    Result ExcelPipeline::run(const ProcessConfig& config) {
        Result result;

        // 配置检查
        if (config.outputDir.empty()) {
            result.errors.push_back("output dir is empty");
            return result;
        }
        if (config.roadKeywords.empty() && config.facilityKeywords.empty()) {
            result.errors.push_back("roadKeywords and facilityKeywords are both empty");
            return result;
        }

        // 构建排除的模板文件集合（不应重复处理模板自身）
        std::unordered_set<std::filesystem::path> excluded;
        if (!config.roadTemplate.empty()) {
            excluded.insert(config.roadTemplate.lexically_normal());
        }
        if (!config.facilityTemplate.empty()) {
            excluded.insert(config.facilityTemplate.lexically_normal());
        }

        reportProgress(Stage::Scanning, "scanning input files");
        std::vector<std::filesystem::path> files;
        std::unordered_set<std::filesystem::path> seen;
        // 扫描混合、道路、设施输入目录，不重复记录
        if (!config.mixedFolder.empty()) {
            scanDirectory(config.mixedFolder, config.includeSubfolders, config.extensions, excluded, seen, files);
        }
        if (!config.roadFolder.empty()) {
            scanDirectory(config.roadFolder, config.includeSubfolders, config.extensions, excluded, seen, files);
        }
        if (!config.facilityFolder.empty()) {
            scanDirectory(config.facilityFolder, config.includeSubfolders, config.extensions, excluded, seen, files);
        }
        result.filesScanned = files.size(); // 记录扫描到的文件数
        reportProgress(Stage::Scanning, "scan complete: " + std::to_string(result.filesScanned) + " files");

        // 按类型对文件分组
        std::vector<std::filesystem::path> roadFiles;
        std::vector<std::filesystem::path> facilityFiles;
        for (const auto& file : files) {
            const FileKind kind = classifyFile(file, config.roadKeywords, config.facilityKeywords);
            switch (kind) {
            case FileKind::Road:
                roadFiles.push_back(file);
                break;
            case FileKind::Facility:
                facilityFiles.push_back(file);
                break;
            case FileKind::Ambiguous:
                result.warnings.push_back("ambiguous type: " + pathToUtf8(file));
                break;
            case FileKind::Unknown:
                result.warnings.push_back("unknown type: " + pathToUtf8(file));
                break;
            }
        }

        // 分组处理文件，将每个线路的数据汇总，routes为 map<线路名, 行列表>
        auto groupFiles = [&](const std::vector<std::filesystem::path>& kindFiles,
            std::unordered_map<std::string, std::vector<Row>>& routes,
            const std::string& kindLabel) {
                std::size_t index = 0;
                for (const auto& file : kindFiles) {
                    ++index;
                    const bool readAllSheets = !config.inputSheetName || config.inputSheetName->empty();
                    std::vector<std::string> sheetNames;
                    if (readAllSheets) {
                        try {
                            sheetNames = reader_.listSheets(file);
                        }
                        catch (const std::exception& ex) {
                            result.errors.push_back("list sheets failed: " + pathToUtf8(file) + " : " + ex.what());
                            continue;
                        }
                        if (sheetNames.empty()) {
                            result.warnings.push_back("no sheets found: " + pathToUtf8(file));
                            continue;
                        }
                    }
                    else {
                        sheetNames.push_back(*config.inputSheetName);
                    }

                    bool fileProcessed = false;
                    for (std::size_t sheetIndex = 0; sheetIndex < sheetNames.size(); ++sheetIndex) {
                        const std::string& sheetName = sheetNames[sheetIndex];
                        const std::string sheetTag = " (sheet " + std::to_string(sheetIndex + 1) + "/" +
                            std::to_string(sheetNames.size()) + ": " + sheetName + ")";

                        reportProgress(Stage::Reading, kindLabel + " reading: " + pathToUtf8(file) + sheetTag,
                            index, kindFiles.size());

                        Sheet sheet;
                        const std::optional<std::string> sheetOpt = sheetName;
                        try {
                            sheet = reader_.readSheet(file, sheetOpt); // 读取sheet
                        }
                        catch (const std::exception& ex) {
                            result.errors.push_back("read failed: " + pathToUtf8(file) + sheetTag + " : " + ex.what());
                            continue;
                        }

                        if (sheet.empty()) {
                            result.warnings.push_back("empty sheet: " + pathToUtf8(file) + sheetTag);
                            continue;
                        }

                        // 查找线路名对应的表头行和列索引
                        const auto headerLocation =
                            findHeaderLocation(sheet, config.headerRowIndex, config.routeNameHeader);
                    if (!headerLocation) {
                        const std::string msg = "route column not found in header: " + pathToUtf8(file) + sheetTag;
                        if (readAllSheets) {
                            result.warnings.push_back(msg);
                            }
                            else {
                                result.errors.push_back(msg);
                            }
                        continue;
                    }
                    const std::size_t headerIndex = headerLocation->first;
                    const std::size_t colIndex = headerLocation->second;
                    const auto headerCandidates = buildNormalizedHeaderCandidates(config.routeNameHeader);
                    reportProgress(Stage::Reading, kindLabel + " header found at row " +
                        std::to_string(headerIndex + 1) + " col " + std::to_string(colIndex + 1) + sheetTag,
                        index, kindFiles.size());
                    const std::size_t headerWidth = lastNonEmptyColumn(sheet[headerIndex]);
                    bool loggedTruncation = false;

                    // 数据起始行
                    const std::size_t minDataStart = headerIndex + 1;
                    std::size_t dataStart = minDataStart;
                    if (config.dataStartRowIndex > 0) {
                        const std::size_t configured = config.dataStartRowIndex - 1;
                        if (configured > minDataStart) {
                            dataStart = configured;
                        }
                    }
                    const std::size_t detectedDataStart =
                        detectDataStartIndex(sheet, headerIndex, colIndex, dataStart, headerCandidates);
                    if (detectedDataStart != dataStart) {
                        reportProgress(Stage::Reading, kindLabel + " auto data start row " +
                            std::to_string(detectedDataStart + 1) + sheetTag, index, kindFiles.size());
                        dataStart = detectedDataStart;
                    }

                        const std::string fileTag = pathToUtf8(file) + sheetTag;
                        const std::vector<std::string> summaryTokens = {
                            u8ToString(u8"合计"),
                            u8ToString(u8"小计"),
                            u8ToString(u8"总计")
                        };
                        bool loggedSummarySkip = false;
                        std::size_t emptyRouteWarned = 0;
                        const std::size_t maxEmptyRouteWarnings = 5;

                        // 遍历数据行
                        for (std::size_t r = dataStart; r < sheet.size(); ++r) {
                            const Row& row = sheet[r];
                            Row rowCopy = row;
                            if (headerWidth > 0 && rowCopy.size() > headerWidth) {
                                if (!loggedTruncation) {
                                    reportProgress(Stage::Reading, kindLabel + " row cols truncated from " +
                                        std::to_string(rowCopy.size()) + " to " + std::to_string(headerWidth) +
                                        " in " + fileTag, index, kindFiles.size());
                                    loggedTruncation = true;
                                }
                                rowCopy.resize(headerWidth);
                            }
                            if (rowIsEmpty(rowCopy)) {
                                continue;
                            }
                            if (rowContainsAny(rowCopy, summaryTokens)) {
                                if (!loggedSummarySkip) {
                                    reportProgress(Stage::Reading, kindLabel +
                                        " summary row skipped " + sheetTag, index, kindFiles.size());
                                    loggedSummarySkip = true;
                                }
                                continue;
                            }
                            if (colIndex >= rowCopy.size()) {
                                result.warnings.push_back("route column missing in row: " + fileTag);
                                continue;
                            }
                            std::string route = normalizeRouteName(cellToString(rowCopy[colIndex]));
                            if (route.empty()) {
                                if (emptyRouteWarned < maxEmptyRouteWarnings) {
                                    result.warnings.push_back("empty route name: " + fileTag +
                                        " row " + std::to_string(r + 1));
                                }
                                ++emptyRouteWarned;
                                continue;
                            }
                            // 同名线路归并
                            routes[route].push_back(std::move(rowCopy));
                        }
                        if (emptyRouteWarned > maxEmptyRouteWarnings) {
                            result.warnings.push_back("empty route name: " + fileTag + " (+" +
                                std::to_string(emptyRouteWarned - maxEmptyRouteWarnings) + " more)");
                        }
                        fileProcessed = true;
                    }
                    if (fileProcessed) {
                        result.filesProcessed += 1;
                    }
                }
            };

        std::unordered_map<std::string, std::vector<Row>> roadRoutes;
        std::unordered_map<std::string, std::vector<Row>> facilityRoutes;

        // 对不同类型分别分组
        groupFiles(roadFiles, roadRoutes, "road");
        groupFiles(facilityFiles, facilityRoutes, "facility");

        std::unordered_set<std::filesystem::path> usedOutputs;

        // 输出每一个线路到文件
        auto writeRoutes = [&](const std::unordered_map<std::string, std::vector<Row>>& routes,
            const std::filesystem::path& templatePath,
            OutputKind kind,
            const std::string& kindLabel) {
                if (routes.empty()) {
                    return;
                }
                if (templatePath.empty()) {
                    result.errors.push_back(kindLabel + " template is empty");
                    return;
                }
                std::error_code ec;
                if (!pathExists(templatePath, ec)) {
                    result.errors.push_back(kindLabel + " template not found: " + pathToUtf8(templatePath));
                    return;
                }

                std::size_t outputStartRow = config.outputStartRowIndex > 0 ? config.outputStartRowIndex : 1;
                try {
                    Sheet templateSheet = reader_.readSheet(templatePath, config.outputSheetName);
                    if (!templateSheet.empty()) {
                        const auto headerLocation =
                            findHeaderLocation(templateSheet, config.headerRowIndex, config.routeNameHeader);
                        if (headerLocation) {
                            const std::size_t headerIndex = headerLocation->first;
                            const std::size_t colIndex = headerLocation->second;
                            const auto headerCandidates = buildNormalizedHeaderCandidates(config.routeNameHeader);
                            const std::size_t outputStartIndexHint =
                                outputStartRow > 0 ? outputStartRow - 1 : headerIndex + 1;
                            const std::size_t detectedStartIndex =
                                detectDataStartIndex(templateSheet, headerIndex, colIndex,
                                    outputStartIndexHint, headerCandidates);
                            const std::size_t detectedStartRow = detectedStartIndex + 1;
                            if (detectedStartRow > outputStartRow) {
                                outputStartRow = detectedStartRow;
                            }
                            reportProgress(Stage::Writing, kindLabel + " output start row " +
                                std::to_string(outputStartRow) + " (template header)",
                                0, routes.size());
                        }
                        else {
                            reportProgress(Stage::Writing, kindLabel + " template header not found; using default start row " +
                                std::to_string(outputStartRow), 0, routes.size());
                        }
                    }
                    else {
                        reportProgress(Stage::Writing, kindLabel + " template is empty; using default start row " +
                            std::to_string(outputStartRow), 0, routes.size());
                    }
                }
                catch (const std::exception& ex) {
                    result.warnings.push_back(kindLabel + " template read failed: " + ex.what());
                }

                std::size_t index = 0;
                for (const auto& entry : routes) {
                    ++index;
                    const std::string& route = entry.first;
                    const std::vector<Row>& rows = entry.second;

                    reportProgress(Stage::Writing, kindLabel + " writing: " + route +
                        " (rows=" + std::to_string(rows.size()) + ")", index, routes.size());

                    const std::filesystem::path outPath =
                        buildOutputPath(config, kind, route, usedOutputs, result);
                    if (outPath.empty()) {
                        continue;
                    }

                    reportProgress(Stage::Writing, kindLabel + " output: " + pathToUtf8(outPath),
                        index, routes.size());
                    reportProgress(Stage::Writing, kindLabel + " template: " + pathToUtf8(templatePath),
                        index, routes.size());
                    try {
                        const auto start = std::chrono::steady_clock::now();
                        reportProgress(Stage::Writing, kindLabel + " opening template",
                            index, routes.size());
                        auto workbook = writer_.openFromTemplate(templatePath, outPath);
                        reportProgress(Stage::Writing, kindLabel + " writing rows",
                            index, routes.size());
                        workbook->writeRows(config.outputSheetName,
                            outputStartRow,
                            config.outputStartColIndex,
                            rows);
                        reportProgress(Stage::Writing, kindLabel + " saving file",
                            index, routes.size());
                        workbook->save();
                        const auto elapsed = std::chrono::steady_clock::now() - start;
                        reportProgress(Stage::Writing, kindLabel + " saved (" +
                            formatDuration(elapsed) + ")", index, routes.size());
                    }
                    catch (const std::exception& ex) {
                        result.errors.push_back("write failed: " + pathToUtf8(outPath) + " : " + ex.what());
                        continue;
                    }

                    result.routesWritten += 1;
                    result.rowsWritten += rows.size();
                }
            };

        // 分别对道路和设施线路输出文件
        writeRoutes(roadRoutes, config.roadTemplate, OutputKind::Road, "road");
        writeRoutes(facilityRoutes, config.facilityTemplate, OutputKind::Facility, "facility");

        reportProgress(Stage::Done, "done");
        return result;
    }

}  // namespace core
