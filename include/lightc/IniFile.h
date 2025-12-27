// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    IniFile.h
 * @brief   IniFile Read Write Class
 *
 *          Light C++ Common Library
 *
 * @author  Hikari Satoh
 * @note    
 *  
 * Copyright (c) 2025 Hikari Satoh
 *               https://3103lab.com
 ******************************************************************************
 */
#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>

namespace LCC
{
    class IniFile {
    public:
        using Section = std::unordered_map<std::string, std::string>;
        using Data    = std::unordered_map<std::string, Section>;

        /******************************************************************************
         * @brief   iniファイル読込
         * @param   strFilename  (in)    開くファイル名
         * @return  結果
         * @retval  true:正常 false:失敗
         * @note
         *****************************************************************************
         */
        bool LoadFromFile(const std::string& strFilename) {
            std::ifstream ifs(strFilename);
            if (!ifs) return false;

            std::string line, currentSection;
            while (std::getline(ifs, line)) {
                Trim(line);
                if (line.empty() || line[0] == ';' || line[0] == '#') continue;

                if (line.front() == '[' && line.back() == ']') {
                    currentSection = line.substr(1, line.length() - 2);
                    Trim(currentSection);
                } else {
                    auto eqPos = line.find('=');
                    if (eqPos == std::string::npos) continue;

                    std::string key = line.substr(0, eqPos);
                    std::string value = line.substr(eqPos + 1);
                    Trim(key);
                    Trim(value);
                    m_mapData[currentSection][key] = value;
                }
            }
            return true;
        }

        /******************************************************************************
         * @brief   iniファイル保存
         * @param   strFilename  (in)    保存するファイル名
         * @return  結果
         * @retval  true:正常 false:失敗
         * @note
         *****************************************************************************
         */
        bool SaveToFile(const std::string& strFilename) const {
            std::ofstream ofs(strFilename);
            if (!ofs) return false;

            for (const auto& [section, kvs] : m_mapData) {
                if (!section.empty()) ofs << "[" << section << "]\n";
                for (const auto& [key, value] : kvs) {
                    ofs << key << "=" << value << "\n";
                }
                ofs << "\n";
            }
            return true;
        }

        /******************************************************************************
         * @brief   値取得
         * @param   strSection       (in)   取得するセクション
         * @param   strKey           (in)   取得するキー
         * @param   strDefaultValue  (in)   デフォルト値
         * @retval  取得文字列
         * @note
         *****************************************************************************
         */
        std::string Get(const std::string& strSection, const std::string& strKey, const std::string& strDefaultValue = "") const {
            auto secIt = m_mapData.find(strSection);
            if (secIt == m_mapData.end()){
                return strDefaultValue;
            }
            auto keyIt = secIt->second.find(strKey);
            return (keyIt != secIt->second.end()) ? keyIt->second : strDefaultValue;
        }

        /******************************************************************************
         * @brief   値保存
         * @param   strSection       (in)   保存するセクション
         * @param   strKey           (in)   保存するキー
         * @param   strValue         (in)   保存する値
         * @return  なし
         * @retval  なし
         * @note
         *****************************************************************************
         */
        void Set(const std::string& strSection, const std::string& strKey, const std::string& strValue) {
            m_mapData[strSection][strKey] = strValue;
        }

        /******************************************************************************
         * @brief   全値の取得
         * @param   なし
         * @return  値のマップ
         * @retval  なし
         * @note
         *****************************************************************************
         */
        const Data& GetAll() const { return m_mapData; }

    private:
        /******************************************************************************
         * @brief   文字列のトリム
         * @param   str       (in/out)   トリムする文字列
         * @return  なし
         * @retval  なし
         * @note
         *****************************************************************************
         */
        static void Trim(std::string& str) {
            const char* whitespace = " \t\r\n";
            str.erase(0, str.find_first_not_of(whitespace));
            str.erase(str.find_last_not_of(whitespace) + 1);
        }

    private:
        Data m_mapData;

    };
}
