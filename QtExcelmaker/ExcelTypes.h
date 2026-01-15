#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace core {

    // 单元格的值类型，支持空、数值、布尔值、字符串
    using CellValue = std::variant<std::monostate, double, bool, std::string>;

    // 行：一行由多个单元格值组成
    using Row = std::vector<CellValue>;

    // Sheet：由多行组成的表
    using Sheet = std::vector<Row>;

    /// @brief 将utf-8字符串视图转换为std::string
    /// @param value utf-8字符串视图
    /// @return std::string
    inline std::string u8ToString(std::u8string_view value) {
        std::string out;
        out.reserve(value.size());
        for (char8_t c : value) {
            out.push_back(static_cast<char>(c));
        }
        return out;
    }

    /// @brief 将单元格的值转换为字符串表示，数值去除多余的0，布尔转为 TRUE/FALSE，空为""
    /// @param value 单元格内容
    /// @return 字符串形式
    inline std::string cellToString(const CellValue& value) {
        // 字符串类型，直接返回
        if (auto text = std::get_if<std::string>(&value)) {
            return *text;
        }
        // 数值类型，转字符串并去除末尾多余的0和小数点
        if (auto num = std::get_if<double>(&value)) {
            std::string s = std::to_string(*num);
            s.erase(s.find_last_not_of('0') + 1);
            if (!s.empty() && s.back() == '.') {
                s.pop_back();
            }
            return s;
        }
        // 布尔类型，转换为 TRUE/FALSE
        if (auto b = std::get_if<bool>(&value)) {
            return *b ? "TRUE" : "FALSE";
        }
        // 空类型，返回空字符串
        return {};
    }

}  // namespace core
