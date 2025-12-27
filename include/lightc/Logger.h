// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    Logger.h
 * @brief   Logger Class
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

#include "MessageDriven.h"
#include "WorkerThreadBase.h"
#include "TimeStamp.h"
#include <mutex>
#include <sstream>
#include <memory>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <thread>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace LCC
{
    #define UN_LOG_INFO_SIZE 256
    #define UN_LOG_TEXT_SIZE 512

    using LogKindIndex = uint8_t;
    using LogKind      = uint32_t;

    inline constexpr uint32_t k_unLogReserveKindBits = 16; // ログ種別ビット数
    inline constexpr uint32_t k_unLogKindBits        = 32; // ログ種別ビット数
    inline constexpr uint32_t k_unLogKindLabelSize   = 16; // ログ種別表示文字列長

    /* ログ種別ビットインデックス */
    inline constexpr LogKindIndex k_unLogKindIndexLccDump        = 0; // デバッグ向け	:データダンプ
    inline constexpr LogKindIndex k_unLogKindIndexLccDetail      = 1; // デバッグ向け	:詳細のデバッグログ
    inline constexpr LogKindIndex k_unLogKindIndexLccDebug       = 2; // デバッグ向け	:通常のデバッグログ
    inline constexpr LogKindIndex k_unLogKindIndexLccSend        = 3; // 通信ログ		:メッセージの送信のログ
    inline constexpr LogKindIndex k_unLogKindIndexLccRecv        = 4; // 通信ログ		:メッセージの受信のログ
    inline constexpr LogKindIndex k_unLogKindIndexLccInfo        = 5; // 情報表示		:一般的な情報表示				ex)ステータス変更など
    inline constexpr LogKindIndex k_unLogKindIndexLccAlert       = 6; // 注意喚起		:通常発生しうる異常のログ		ex)NWトラブルなどで起きる受信異常など
    inline constexpr LogKindIndex k_unLogKindIndexLccError       = 7; // 重大問題		:通常発生しない重大問題のログ	ex)メモリエラーやロジックが間違っているなど

    inline constexpr LogKindIndex k_unLogKindIndexAppDump        = 8; // デバッグ向け	:データダンプ
    inline constexpr LogKindIndex k_unLogKindIndexAppDetail      = 9; // デバッグ向け	:詳細のデバッグログ
    inline constexpr LogKindIndex k_unLogKindIndexAppDebug       = 10; // デバッグ向け	:通常のデバッグログ
    inline constexpr LogKindIndex k_unLogKindIndexAppSend        = 11; // 通信ログ		:メッセージの送信のログ
    inline constexpr LogKindIndex k_unLogKindIndexAppRecv        = 12; // 通信ログ		:メッセージの受信のログ
    inline constexpr LogKindIndex k_unLogKindIndexAppInfo        = 13; // 情報表示		:一般的な情報表示				ex)ステータス変更など
    inline constexpr LogKindIndex k_unLogKindIndexAppAlert       = 14; // 注意喚起		:通常発生しうる異常のログ		ex)NWトラブルなどで起きる受信異常など
    inline constexpr LogKindIndex k_unLogKindIndexAppError       = 15; // 重大問題		:通常発生しない重大問題のログ	ex)メモリエラーやロジックが間違っているなど


    /******************************************************************************
     * @brief   Singleton Logger Class
     *          
     * @note    ワーカースレッドを使用した非同期ログ出力クラス。
     *          ログのファイル出力、ログファイルの自動ローテーションを提供します。
     *           
     *****************************************************************************/
    class Logger : public MessageDriven<std::string>
    {
    public:
        /******************************************************************************
         * @brief   Loggerインスタンスを取得する関数（シングルトンパターン）
         * @param   なし
         * @return  Loggerクラスの唯一のインスタンスへの参照
         * @retval  なし
         * @note
         *****************************************************************************/
        static Logger& Instance() {
            static Logger s_instance;
            return s_instance;
        }

        /******************************************************************************
         * @brief   ログ出力用ワーカースレッドの開始関数
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    m_mutex による排他制御を行い、ワーカースレッドが未生成の場合に生成・開始する
         *****************************************************************************/
        void Start() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_spWorker) {
                m_spWorker = std::make_unique<WorkerThreadBase<std::string>>(
                    std::shared_ptr<MessageDriven<std::string>>(&Instance(), [](void*) {}));
                m_spWorker->Start();
            }
        }

        /******************************************************************************
         * @brief   ログ出力用ワーカースレッドの停止関数
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    ワーカースレッドが生成済みの場合、停止してリソースを解放する
         *****************************************************************************/
        void Stop() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_spWorker) {
                m_spWorker->Stop();
                m_spWorker.reset();
            }
        }

        /******************************************************************************
         * @brief   ログマスクの設定関数
         * @param   unMask (in)  ログ出力対象のビットフラグ
         * @return  なし
         * @retval  なし
         * @note
         *****************************************************************************/
        void SetLogMask(LogKind unMask) { m_unLogMask = unMask; }

        /******************************************************************************
         * @brief   ログファイルのプレフィックスを設定する関数
         * @param   strPrefix (in)  ログファイル名のプレフィックス文字列
         * @return  なし
         * @retval  なし
         * @note
         *****************************************************************************/
        void SetLogFilePrefix(const std::string& strPrefix) { m_strFilePrefix = strPrefix; }

        /******************************************************************************
         * @brief   ログディレクトリを設定する関数
         * @param   strLogDir (in)  ログディレクトリ文字列
         * @return  なし
         * @retval  なし
         * @note
         *****************************************************************************/
        void SetLogDir(const std::string& strLogDir) { m_strLogDir = strLogDir; }

        /******************************************************************************
         * @brief   ログファイルの有効期限（秒）の設定関数
         * @param   sec (in)  有効期限（秒）
         * @return  なし
         * @retval  なし
         * @note    期限切れログファイルは CleanupOldLogs() で削除対象となる
         *****************************************************************************/
        void SetFileExpireSeconds(uint64_t sec) { m_unExpireSec = sec; }

        /******************************************************************************
         * @brief   ログメッセージの出力関数
         * @param   level (in)    ログレベル
         * @param   message (in)  ログメッセージ文字列
         * @return  なし
         * @retval  なし
         * @note    ログマスクに基づき、出力対象の場合のみメッセージをキューに投稿する
         *****************************************************************************/
        void Write(LogKind unKind, const std::string& message) {
            if ((m_unLogMask & unKind) != 0) {
                Post(message);
            }
        }

        /******************************************************************************
         * @brief   コンテキスト付きフォーマットログ出力関数
         * @param   unLogKindIndex (in)   ログ種別
         * @param   pszFileName    (in)   ソースファイル名
         * @param   nLine          (in)   ソース行番号
         * @param   pszFunc        (in)   関数名
         * @param   fmt            (in)   フォーマット文字列
         * @param   ...            (in)   可変引数（フォーマット文字列に対応する引数群）
         * @return  なし
         * @retval  なし
         * @note    ログマスクにより出力対象の場合のみ、日時、ログレベル、ファイル情報等を付加してログを出力する
         *****************************************************************************/
        void WriteFormatWithContext(LogKindIndex unLogKindIndex,
                                    const char* pszFileName, int nLine, const char* pszFunc,
                                    const char* fmt, ...) {
            const uint32_t unKind = 1u << unLogKindIndex;
            if ((m_unLogMask & unKind) == 0) return;

            const char* pszLabel = m_szLogKindLabel[unLogKindIndex];
            char szLogText[UN_LOG_TEXT_SIZE] = {0};
            va_list args;
            va_start(args, fmt);
            std::vsnprintf(szLogText, sizeof(szLogText), fmt, args);
            va_end(args);

            pszFileName = (strrchr(pszFileName, '/') ? strrchr(pszFileName, '/') + 1 : pszFileName);
            pszFileName = (strrchr(pszFileName, '\\') ? strrchr(pszFileName, '\\') + 1 : pszFileName);

            char szInfo[UN_LOG_INFO_SIZE] = {0};
            std::snprintf(szInfo, sizeof(szInfo), "%s,%s",
                TimeStamp::Now().ToString().c_str(), pszLabel);

            std::ostringstream ssTid;
            ssTid << std::this_thread::get_id();

            char szFileInfo[UN_LOG_INFO_SIZE] = {0};
            std::snprintf(szFileInfo, sizeof(szFileInfo), "%s,%s:%d,thread=%s",
                          pszFunc, pszFileName, nLine, ssTid.str().c_str());

            std::stringstream ss;
            ss << szInfo << "," << szLogText << "," << szFileInfo;

            Write(unKind, ss.str());
        }

        /******************************************************************************
         * @brief   ログ種別ラベル登録
         * @param   unLogKindIndex (in)   ログ種別インデックス
         * @param   pszLabel       (in)   ラベル
         * @return  処理結果
         * @retval  true:成功 false:失敗
         * @note    カスタムログ種別のラベルの登録を行います。
         *          ライブラリ標準のログは0～(k_unLogReserveKindBits-1)のインデックスを使用します。
         *          カスタムログ種別はk_unLogReserveKindBits～(k_unLogKindBits-1)のインデックスを使用してください。
         *****************************************************************************/
        bool RegisterCustomLogKindLabel(LogKindIndex unLogKindIndex, const char* pszLabel) {
            if (pszLabel == nullptr) return false;

            // 0..15はLCC予約
            if (unLogKindIndex < static_cast<LogKindIndex>(k_unLogReserveKindBits)) return false;

            // 32以上は範囲外
            if (unLogKindIndex >= static_cast<LogKindIndex>(k_unLogKindBits)) return false;

            return RegisterLogKindLabelByIndex(unLogKindIndex, pszLabel);
        }

    protected:
        /******************************************************************************
         * @brief   ログメッセージ受信時の処理関数
         * @param   msg (in)  受信したログメッセージ
         * @return  なし
         * @retval  なし
         * @note    ファイル出力（ログファイルのオープン、切替、古いログの削除）および標準出力に出力する
         *****************************************************************************/
        void OnMessage(const std::string& msg) override {
            std::lock_guard<std::mutex> lock(m_fileMutex);

            std::string strNewPath = MakeLogFilePath();
            if (strNewPath != m_strCurrentLogFilePath) {
                if (m_ofs.is_open()) m_ofs.close();
                m_strCurrentLogFilePath = strNewPath;
                m_ofs.open(m_strCurrentLogFilePath, std::ios::app);
                CleanupOldLogs();
            }

            if (m_ofs.is_open()) {
                m_ofs << msg << std::endl;
            }
            if (m_unConsoleOut) {
                std::cout << msg << std::endl;
            }
        }

    private:
        /******************************************************************************
         * @brief   コンストラクタ（非公開）
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    シングルトンパターンのため外部からの生成を禁止する
         *****************************************************************************/
        Logger() :  m_unLogMask(0xFFFFFFFF),
                    m_strLogDir("../log")
        {
            InitLogKindLabels();
        }

        /******************************************************************************
         * @brief   デストラクタ（非公開）
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    ログワーカースレッドの停止を行う
         *****************************************************************************/
        ~Logger() override { Stop(); }

        /******************************************************************************
         * @brief   ログ種別ラベル登録
         * @param   unLogKindIndex (in)   ログ種別インデックス
         * @param   pszLabel       (in)   ラベル
         * @return  処理結果
         * @retval  true:成功 false:失敗
         * @note    ライブラリ標準ログ種別のラベルの登録を行います。
         *          0～(k_unLogReserveKindBits-1)のインデックスを使用します。
         *****************************************************************************/
        bool RegisterLogKindLabelByIndex(LogKindIndex unLogKindIndex, const char* pszLabel) {
            if (pszLabel == nullptr) return false;
            if (unLogKindIndex >= k_unLogKindBits) return false;

            std::snprintf(m_szLogKindLabel[unLogKindIndex],
                          k_unLogKindLabelSize,
                          "%-15.15s", pszLabel);
            m_szLogKindLabel[unLogKindIndex][k_unLogKindLabelSize - 1] = '\0';
            return true;
        }

        /******************************************************************************
         * @brief   標準ログ種別ラベル登録
         * @param   unLogKindIndex (in)   ログ種別インデックス
         * @param   pszLabel       (in)   ラベル
         * @return  処理結果
         * @retval  true:成功 false:失敗
         * @note    ライブラリ標準ログ種別のラベルの登録を行います。
         *          0～(k_unLogReserveKindBits-1)のインデックスを使用します。
         *****************************************************************************/
        void InitLogKindLabels() {
            // 全部 UNDEF で埋める
            for (LogKindIndex i = 0; i < static_cast<LogKindIndex>(k_unLogKindBits); ++i) {
                RegisterLogKindLabelByIndex(i, "UNDEF");
            }

            // LCC 標準(0..15のうち使用分だけ)
            RegisterLogKindLabelByIndex(k_unLogKindIndexLccDump,   "LCC_DUMP");
            RegisterLogKindLabelByIndex(k_unLogKindIndexLccDetail, "LCC_DETAIL");
            RegisterLogKindLabelByIndex(k_unLogKindIndexLccDebug,  "LCC_DEBUG");
            RegisterLogKindLabelByIndex(k_unLogKindIndexLccSend,   "LCC_SEND");
            RegisterLogKindLabelByIndex(k_unLogKindIndexLccRecv,   "LCC_RECV");
            RegisterLogKindLabelByIndex(k_unLogKindIndexLccInfo,   "LCC_INFO");
            RegisterLogKindLabelByIndex(k_unLogKindIndexLccAlert,  "LCC_ALERT");
            RegisterLogKindLabelByIndex(k_unLogKindIndexLccError,  "LCC_ERROR");

            RegisterLogKindLabelByIndex(k_unLogKindIndexAppDump,   "DUMP");
            RegisterLogKindLabelByIndex(k_unLogKindIndexAppDetail, "DETAIL");
            RegisterLogKindLabelByIndex(k_unLogKindIndexAppDebug,  "DEBUG");
            RegisterLogKindLabelByIndex(k_unLogKindIndexAppSend,   "SEND");
            RegisterLogKindLabelByIndex(k_unLogKindIndexAppRecv,   "RECV");
            RegisterLogKindLabelByIndex(k_unLogKindIndexAppInfo,   "INFO");
            RegisterLogKindLabelByIndex(k_unLogKindIndexAppAlert,  "ALERT");
            RegisterLogKindLabelByIndex(k_unLogKindIndexAppError,  "ERROR");
        }

        /******************************************************************************
         * @brief   ログファイルパスを生成する関数
         * @param   なし
         * @return  std::string - 生成されたログファイルパス
         * @retval  "<プレフィックス>_<YYYYMMDD>_<HH>.txt"
         * @note    std::chrono::zoned_time と std::format を利用して、
         *          現在のローカル時刻に基づくファイル名を生成する。
         *****************************************************************************/
        std::string MakeLogFilePath() {
            return m_strLogDir + m_strFilePrefix + "_" + TimeStamp::Now().ToString("%Y%m%d_%H") + ".txt";
        }

        /******************************************************************************
         * @brief   古いログファイルの削除を行う関数
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    m_unExpireSec の設定値に基づき、一定時間経過したログファイルを削除する
         *****************************************************************************/
        void CleanupOldLogs() {
            if (m_unExpireSec == 0 || m_strFilePrefix.empty()) return;

            auto now = std::chrono::system_clock::now();
            std::filesystem::path dir = std::filesystem::path(MakeLogFilePath()).parent_path();
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;

                auto path = entry.path();
                auto fname = path.filename().string();
                std::string want = m_strFilePrefix + "_";
                if (fname.rfind(want, 0) != 0) continue;

                auto ftime = std::filesystem::last_write_time(path);
                auto fclock = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

                auto age = std::chrono::duration_cast<std::chrono::seconds>(now - fclock).count();
                if (age > static_cast<int64_t>(m_unExpireSec)) {
                    std::error_code ec;
                    std::filesystem::remove(path, ec);
                }
            }
        }

    private:
        std::mutex m_mutex;
        std::unique_ptr<WorkerThreadBase<std::string>> m_spWorker;
        std::atomic_uint32_t m_unLogMask;
        char          m_szLogKindLabel[k_unLogKindBits][k_unLogKindLabelSize] = {};

        std::string   m_strFilePrefix;
        std::string   m_strLogDir;
        uint64_t      m_unExpireSec  = 0;
        bool          m_unConsoleOut = false;
        std::string   m_strCurrentLogFilePath;
        std::ofstream m_ofs;
        std::mutex    m_fileMutex;
    };
}

/******************************************************************************
 * @brief   カテゴリ別ログマクロ（可変引数対応・自動コンテキスト付き）
 * @param   なし
 * @return  なし
 * @retval  なし
 * @note    各ログレベルに対応するログ出力関数を簡潔に呼び出すためのマクロ群
 *****************************************************************************/
// LCCライブラリ用標準
#define LCC_LOG_DUMP(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexLccDump, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LCC_LOG_DETAIL(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexLccDetail, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LCC_LOG_DEBUG(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexLccDebug, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LCC_LOG_INFO(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexLccInfo, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LCC_LOG_SEND(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexLccSend, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LCC_LOG_RECV(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexLccRecv, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LCC_LOG_ALERT(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexLccAlert, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LCC_LOG_ERROR(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexLccError, __FILE__, __LINE__, __func__, __VA_ARGS__)

// 標準
#define LOG_DUMP(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexAppDump, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_DETAIL(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexAppDetail, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_DEBUG(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexAppDebug, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_INFO(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexAppInfo, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_SEND(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexAppSend, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_RECV(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexAppRecv, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_ALERT(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexAppAlert, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_ERROR(...) \
    Logger::Instance().WriteFormatWithContext(k_unLogKindIndexAppError, __FILE__, __LINE__, __func__, __VA_ARGS__)