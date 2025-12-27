// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    TimeStamp.h
 * @brief   C++/Common Library TimeStamp Class
 * @author  Hikari Satoh
 * @note    
 *  
 * Copyright (c) 2025 Hikari Satoh
 *               https://3103lab.com
 ******************************************************************************
 */
#pragma once
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace LCC
{
    class TimeStamp {
    public:
        using clock        = std::chrono::system_clock;
        using microseconds = std::chrono::microseconds;
        using time_point   = std::chrono::time_point<clock, microseconds>;

        /******************************************************************************
         * @brief   現在時刻を取得
         * @param   なし
         * @return  現在時刻のタイムスタンプ
         * @retval  なし
         * @note    
         ******************************************************************************
         */
        static TimeStamp Now() {
            auto tp = std::chrono::time_point_cast<microseconds>(clock::now());
            return TimeStamp(tp);
        }

        /******************************************************************************
         * @brief   文字列からタイムスタンプへ変換
         * @param   strTime  (in)    yyyy/MM/dd HH:mm:ss:ffffffフォーマットの文字列
         * @return  変換したタイムスタンプ
         * @retval  なし
         * @note    変換に失敗した場合にはstd::invalid_argument例外が発生します
         ******************************************************************************
         */
        static TimeStamp FromString(const std::string& strTime) {
            if (strTime.size() != 26 || strTime[4] != '/' || strTime[7] != '/' || strTime[10] != ' ' || strTime[13] != ':' || strTime[16] != ':' || strTime[19] != ':') {
                throw std::invalid_argument("Invalid format, expected yyyy/MM/dd HH:mm:ss:ffffff");
            }
            std::tm tm{};
            std::istringstream ss(strTime.substr(0, 19));
            ss >> std::get_time(&tm, "%Y/%m/%d %H:%M:%S");
            if (ss.fail()) {
                throw std::invalid_argument("Failed to parse date/time");
            }
            int micro = std::stoi(strTime.substr(20, 6));
            std::time_t tt = to_time_t_utc(tm);
            auto tp = std::chrono::time_point_cast<microseconds>(clock::from_time_t(tt)) + microseconds(micro);
            return TimeStamp(tp);
        }

        /******************************************************************************
         * @brief   タイムスタンプから文字列へ変換(ローカルタイム)
         * @param   なし
         * @return  変換したyyyy/MM/dd HH:mm:ss:ffffffフォーマットの文字列
         * @retval  なし
         * @note    
         ******************************************************************************
         */
        std::string ToString() const {
            std::time_t tt = clock::to_time_t(time_point_);
            std::tm tm = to_tm_local(tt);
            std::ostringstream ss;
            ss << std::put_time(&tm, "%Y/%m/%d %H:%M:%S");
            auto usec = time_point_.time_since_epoch() % microseconds(1000000);
            ss << ':' << std::setw(6) << std::setfill('0') << usec.count();
            return ss.str();
        }

        /******************************************************************************
         * @brief   タイムスタンプから文字列へ変換(ローカルタイム)
         * @param   pszFormat (in) フォーマット
         * @return  指定されたフォーマットの文字列
         * @retval  なし
         * @note    フォーマット例 "%Y/%m/%d %H:%M:%S"
         ******************************************************************************
         */
        std::string ToString(const char* pszFormat) const {
            std::time_t tt = clock::to_time_t(time_point_);
            std::tm tm = to_tm_local(tt);
            std::ostringstream ss;
            ss << std::put_time(&tm, pszFormat);
            return ss.str();
        }

        /******************************************************************************
         * @brief   タイムスタンプからepochマイクロ秒整数への変換
         * @param   なし
         * @return  epochマイクロ秒整数
         * @retval  なし
         * @note
         ******************************************************************************
         */
        int64_t ToEpochMicro() const {
            return time_point_.time_since_epoch().count();
        }

        /******************************************************************************
         * @brief   epochマイクロ秒整数からタイムスタンプからへの変換
         * @param   epochマイクロ秒整数
         * @return  タイムスタンプ
         * @retval  なし
         * @note
         ******************************************************************************
         */
        static TimeStamp FromEpochMicro(int64_t us) {
            return TimeStamp(time_point(microseconds(us)));
        }

        /******************************************************************************
         * @brief   2時刻の差分の取得(秒)
         * @param   比較するタイムスタンプ
         * @return  差分(秒)
         * @retval  なし
         * @note    
         ******************************************************************************
         */
        int64_t DiffSeconds(const TimeStamp& other) const {
            auto d = time_point_ - other.time_point_;
            auto sec = std::chrono::duration_cast<std::chrono::seconds>(d);
            return sec.count();
        }

        /******************************************************************************
         * @brief   2時刻の差分の取得(ミリ秒)
         * @param   比較するタイムスタンプ
         * @return  差分(ミリ秒)
         * @retval  なし
         * @note    
         ******************************************************************************
         */
        int64_t DiffMilliseconds(const TimeStamp& other) const {
            auto d = time_point_ - other.time_point_;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d);
            return ms.count();
        }

    private:
        /******************************************************************************
         * @brief   time_point を直接指定して TimeStamp を生成する内部コンストラクタ
         * @param   tp (in)  std::chrono::system_clock::time_point
         * @note    外部からの直接生成を防ぐため private とし、
         *          Now() や FromTimeT() などのファクトリ経由でのみ使用する。
         ******************************************************************************
         */
        explicit TimeStamp(const time_point& tp) : time_point_(tp) {}

        /******************************************************************************
         * @brief   UTC基準の std::tm を time_t に変換する
         * @param   tm (in)  UTC表現の std::tm 構造体
         * @return  std::time_t
         * @note    プラットフォーム差異を吸収するため、
         *          Windows では _mkgmtime、
         *          POSIX 環境では timegm を使用する。
         *          tm は UTC として解釈され、ローカルタイム補正は行われない。
         ******************************************************************************
         */
        static std::time_t to_time_t_utc(std::tm tm) {
            #ifdef _WIN32
                return _mkgmtime(&tm);
            #else
                return timegm(&tm);
            #endif
        }
        /******************************************************************************
         * @brief   UTC基準で time_t を std::tm に変換する
         * @param   t (in)  UNIX epoch 基準の時刻（UTC）
         * @return  std::tm UTC表現の時刻構造体
         * @note    スレッドセーフな API を使用し、
         *          Windows では gmtime_s、
         *          POSIX 環境では gmtime_r を使用する。
         ******************************************************************************
         */
        static std::tm to_tm_utc(std::time_t t) {
            std::tm tm{};
            #ifdef _WIN32
                gmtime_s(&tm, &t);
            #else
                gmtime_r(&t, &tm);
            #endif
            return tm;
        }

        /******************************************************************************
         * @brief   ローカルタイム基準で time_t を std::tm に変換する
         * @param   t (in)  UNIX epoch 基準の時刻
         * @return  std::tm ローカルタイム表現の時刻構造体
         * @note    環境のタイムゾーン設定を反映する。
         *          スレッドセーフな API を使用し、
         *          Windows では localtime_s、
         *          POSIX 環境では localtime_r を使用する。
         ******************************************************************************
         */
        static std::tm to_tm_local(std::time_t t) {
            std::tm tm{};
            #ifdef _WIN32
                localtime_s(&tm, &t);
            #else
                localtime_r(&t, &tm);
            #endif
            return tm;
        }

    private:
        time_point time_point_;
    };
}