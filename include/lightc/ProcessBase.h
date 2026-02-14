// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    ProcessFactoryBase.h
 * @brief   Process Factory Base Class
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
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <stdexcept>

#include <lightc/EventDriven.h>
#include <lightc/ProcessEvent.h>
#include <lightc/TimerManager.h>
#include <lightc/Logger.h>
#include <lightc/IniFile.h>
#include <lightc/TimeStamp.h>
#include <lightc/Signal.h>

namespace LCC
{
	
using fnMessageHandler = std::function<void(const MessageEvent&)>;
using fnTimerHandler   = std::function<void(const TimerEvent&)>;
using fnSignalHandler  = std::function<void(const SignalEvent&)>;

using MessageHandlerMap = std::unordered_map<std::string, std::function<void(const MessageEvent&)>>;
using TimerHandlerMap   = std::unordered_map<TimerId,     std::function<void(const TimerEvent&)>>;
using SignalHandlerMap  = std::unordered_map<SignalNo,    std::function<void(const SignalEvent&)>>;

class ProcessBase : public LCC::EventDriven<LCC::ProcessEvent>, public std::enable_shared_from_this<ProcessBase>
{
public:
    /******************************************************************************
     * @brief   コンストラクタ
     * @arg     なし
     * @return  なし
     * @note    
     *****************************************************************************/
    ProcessBase(const std::string& strIniFile = "config.ini")
        : m_strIniFile(strIniFile),
          m_bRunning(true)
    {

    }

    /******************************************************************************
     * @brief   デストラクタ
     * @arg     なし
     * @return  なし
     * @note    プロセス停止処理、MQ切断、受信スレッドの終了を行う
     *****************************************************************************/
    virtual ~ProcessBase()
    {
        Shutdown();
        Logger::Instance().Stop();
    }

    /******************************************************************************
     * @brief   プロセス初期化
     * @arg		argc		(in)	引数の数
     * @arg		argv		(in)	引数配列
     * @return  なし
     * @note
     *****************************************************************************/
    void Initialize(int argc, const char* argv[])
    {
        vArgumentAnalyze(argc, argv);
        vLoadConfig();
		std::shared_ptr<ProcessBase> spThis = shared_from_this();
        m_pcTimerManager = std::make_unique<TimerManager<ProcessEvent>>(spThis);
        
        vOnInitialize();
    }

    /******************************************************************************
     * @brief   プロセスの開始処理を行う関数
     * @arg     なし
     * @return  なし
     * @note    MessageDrivenの開始を行う
     *****************************************************************************/
    void Start()
    {
        std::thread(&ProcessBase::vSignalWaitThread, this).detach();
        
        vRunProcess();
    }

    /******************************************************************************
     * @brief   プロセスの停止処理を行う関数
     * @arg     なし
     * @return  なし
     * @note    MessageDrivenのシャットダウンを行う
     *****************************************************************************/
    void Stop()
    {
		    m_bRunning.store(false);
        // シグナル待受スレッドのブロッキングを解除する
        LCC::Signal::Raise(SIGUSR2);

        vOnStop();
        Shutdown(); // MessageDriven のシャットダウン
    }

    /******************************************************************************
     * @brief   メッセージハンドラの登録を行う関数
     * @arg     strEventName (in) 登録するイベント名
     * @arg     fnHandler    (in) 登録するハンドラ関数
     * @return  なし
	 * @note    同一イベント名が登録されていた場合は上書きする
     *****************************************************************************/
    void RegisterMessageHandler(
            const std::string& strEventName, fnMessageHandler fnHandler)
    {
        LCC_LOG_INFO("Register Message handler for EventName[%s].", strEventName.c_str());
        {
            std::lock_guard<std::mutex> lock(m_cMessageHandlerMutex);
            m_mapMessageHandler.emplace(strEventName, fnHandler);
        } // mutex unlock provides release fence
        
        // シグナル待受スレッドのブロッキングを解除する
        LCC::Signal::Raise(SIGUSR2);
    }

    /******************************************************************************
     * @brief   タイマー登録関数
     * @arg     unTimerId (in) 登録するタイマーID
     * @arg     fnHandler (in) 登録するハンドラ関数
     * @return  なし
     * @note    同一IDが登録されていた場合は上書きする
     *****************************************************************************/
    void RegisterTimer(TimerId unTimerId, fnTimerHandler fnHandler)
    {
        LCC_LOG_INFO("Register Timer handler for TimerId[%lu].", unTimerId);
        std::lock_guard<std::mutex> lock(m_cTimerHandlerMutex);
        m_mapTimerHandler.emplace(unTimerId, fnHandler);
    }
    
    /******************************************************************************
     * @brief   シグナルハンドラの登録を行う関数
     * @arg     snSignal (in) 登録するシグナル番号
     * @arg     fn       (in) 登録するハンドラ関数
     * @return  なし
     * @note    同一シグナル番号が登録されていた場合は上書きする
	 *          SIGUSR2は内部で使用されているため登録できません
	 * @throw   std::logic_error SIGUSR2を登録しようとした場合
	 *****************************************************************************/
    void RegisterSignalHandler(SignalNo snSignal, fnSignalHandler fnHandler) {
        if (snSignal == SIGUSR2) {
            throw std::logic_error("Cannot register SIGUSR2 handler: it is reserved for internal use");
        }
        
        LCC_LOG_INFO("Register Signal handler for SignalNo[%lu].", snSignal);
        {
            std::lock_guard<std::mutex> lk(m_cSignalHandlerMutex);
            m_mapSignalHandler.emplace(snSignal, fnHandler);
        }
        
        // シグナル待受スレッドのブロッキングを解除する
        LCC::Signal::Raise(SIGUSR2);
    }

	// IniFileクラスのインスタンスを取得する
    IniFile& GetIniFile() { return m_cIniFile; }
	bool IsRunning() const { return m_bRunning.load(); }

protected:
    /******************************************************************************
     * @brief   初期化処理(純粋仮想関数)
     * @arg     なし
     * @return  なし
     * @note    アプリケーション側の初期化処理を記述してください
     *****************************************************************************/
    virtual void vOnInitialize() = 0;

    /******************************************************************************
     * @brief   プロセス停止処理(純粋仮想関数)
     * @arg     なし
     * @return  なし
     * @note    アプリケーション側の停止処理を記述してください
     *****************************************************************************/
    virtual void vOnStop() = 0;

protected:
    /******************************************************************************
     * @brief   イベント処理関数（MessageDrivenからの呼び出し）
     * @arg     cEvent (in) 受信イベント
     * @return  なし
     * @note    登録された各ハンドラを呼び出す
     *****************************************************************************/
    inline void vOnEvent(const ProcessEvent& cEvent) override
    {
        std::visit([this](auto&& cEvent) {
            using T = std::decay_t<decltype(cEvent)>;

            if constexpr (std::is_same_v<T, MessageEvent>) {
                vDispatchMessage(cEvent);
            }
            else if constexpr (std::is_same_v<T, TimerEvent>) {
                vDispatchTimer(cEvent);
            }
            else if constexpr (std::is_same_v<T, SignalEvent>) {
                vDispatchSignal(cEvent);
             }
            else {
                LCC_LOG_ERROR("Unknown event type received.");
            }
        }, cEvent);
    }

    /******************************************************************************
     * @brief   メッセージイベントディスパッチ
     * @arg     cEvent (in) 受信メッセージイベント
     * @return  なし
     * @note    登録された各ハンドラを呼び出す
     *****************************************************************************/
    inline void vDispatchMessage(const MessageEvent& cEvent)
    {
		std::string strEventName = cEvent.strEventName;

        std::lock_guard<std::mutex> lock(m_cMessageHandlerMutex);
        const auto& itr = m_mapMessageHandler.find(strEventName);
        if (itr == m_mapMessageHandler.end()) {
            LCC_LOG_ALERT("No handler registered for EventName[%s]", strEventName.c_str());
        }

        LCC_LOG_DEBUG("EventName[%s] Handler Begin.", strEventName.c_str());
        TimeStamp tmBegin = TimeStamp::Now();
        const auto& handler = itr->second;
        handler(cEvent);

        TimeStamp tmEnd = TimeStamp::Now();
        int64_t snDiffTime = tmEnd.DiffMilliseconds(tmBegin);
        LCC_LOG_DEBUG("EventName[%s] Handler End. ElapsedTime[%ld]ms", strEventName.c_str(), snDiffTime);
    }

    /******************************************************************************
     * @brief   タイマーイベントディスパッチ
     * @arg     cEvent (in) 受信タイマーイベント
     * @return  なし
     * @note    登録された各ハンドラを呼び出す
     *****************************************************************************/
    inline void vDispatchTimer(const TimerEvent& cEvent)
    {
        TimerId unTimerId = cEvent.unTimerId;

        std::lock_guard<std::mutex> lock(m_cTimerHandlerMutex);
        const auto& itr = m_mapTimerHandler.find(unTimerId);
        if (itr == m_mapTimerHandler.end()) {
            LCC_LOG_ALERT("No handler registered for TimerId[%lu]", unTimerId);
            return;
        }

        LCC_LOG_DEBUG("TimerId[%lu] Handler Begin.", unTimerId);
        TimeStamp tmBegin = TimeStamp::Now();
        const auto& handler = itr->second;
        handler(cEvent);

        TimeStamp tmEnd = TimeStamp::Now();
        int64_t snDiffTime = tmEnd.DiffMilliseconds(tmBegin);
        LCC_LOG_DEBUG("TimerId[%lu] Handler End.ElapsedTime[%ld]ms", unTimerId, snDiffTime);

    }

    /******************************************************************************
     * @brief   シグナルイベントディスパッチ
     * @arg     ev (in) 受信シグナルイベント
     * @return  なし
     * @note    登録された各ハンドラを呼び出す
	 *****************************************************************************/
    void vDispatchSignal(const SignalEvent& cEvent) {

        TimerId snSignal = cEvent.snSignal;

        std::lock_guard<std::mutex> lock(m_cTimerHandlerMutex);
        const auto& itr = m_mapSignalHandler.find(snSignal);
        if (itr == m_mapSignalHandler.end()) {
            LCC_LOG_ALERT("No handler registered for TimerId[%lu]", snSignal);
            return;
        }

        LCC_LOG_DEBUG("Signal[%ld] Handler Begin.", snSignal);
        TimeStamp tmBegin = TimeStamp::Now();
        const auto& handler = itr->second;
        handler(cEvent);

        TimeStamp tmEnd = TimeStamp::Now();
        int64_t snDiffTime = tmEnd.DiffMilliseconds(tmBegin);
        LCC_LOG_DEBUG("Signal[%ld] Handler End.ElapsedTime[%ld]ms", snSignal, snDiffTime);

    }

    /******************************************************************************
     * @brief   Exception発生時のロギング
     * @arg     なし
     * @return  なし
     * @note    
     *****************************************************************************/
    virtual void LogOnEventException(const std::exception& ex) override{
        LCC_LOG_ERROR("Exception in OnEvent(): %s", ex.what());
    }

    /******************************************************************************
     * @brief   Exception発生時のロギング
     * @arg     なし
     * @return  なし
     * @note    
     *****************************************************************************/
    virtual void LogOnEventException() override {
        LCC_LOG_ERROR("Unknown Exception in OnEvent()");
    }

    /******************************************************************************
     * @brief   タイマー登録関数（ワンショット）
     * @arg     unTimerId (in) タイマーID（ユニーク識別子）
     * @arg     unDelayMs (in) 遅延時間（ミリ秒）
     * @return  なし
     * @note    同一IDが登録されていた場合は置き換える
     *****************************************************************************/
    void vStartTimer(TimerId unTimerId, uint64_t unDelayMs) {
        if (m_pcTimerManager) {
            LCC_LOG_DEBUG("TimerId[%lu] Start.", unTimerId);
            std::unique_ptr<ProcessEvent> upEvent = std::make_unique<ProcessEvent>(TimerEvent(unTimerId));
            m_pcTimerManager->StartTimer(unTimerId, unDelayMs, std::move(upEvent));
        }
    }

    /******************************************************************************
     * @brief   タイマー停止関数
     * @arg     unTimerId (in) タイマーID（ユニーク識別子）
     * @return  なし
     * @note    
     *****************************************************************************/
    void vStopTimer(TimerId unTimerId) {
        if (m_pcTimerManager) {
            LCC_LOG_DEBUG("TimerId[%lu] Stop.", unTimerId);
            m_pcTimerManager->StopTimer(unTimerId);
        }
    }

    /******************************************************************************
     * @brief   シグナル待ち受けスレッド
     * @arg     なし
     * @return  なし
     * @note    シグナルを待ち受け、受信したらイベントとしてPostする
	 *****************************************************************************/
    void vSignalWaitThread()
    {
        while( m_bRunning.load() )
		{
			std::list<SignalNo> listSignal;
            {
                std::lock_guard<std::mutex> lk(m_cSignalHandlerMutex);
                for (auto& [snSignal, spClientInfo] : m_mapSignalHandler) {
                    listSignal.push_back(snSignal);
                }
            }
            
            // SIGUSR2をハンドラ更新通知用に追加
            listSignal.push_back(SIGUSR2);

            SignalNo snSignal = Signal::Wait(listSignal);

            // SIGUSR2の場合はハンドラ更新なので、イベントをPostせずに次のループへ
            if (snSignal == SIGUSR2) {
                continue;
            }

            SignalEvent cSignalEvent{};
            cSignalEvent.snSignal = snSignal;
            Post(cSignalEvent);
        }
    }

private:
    /******************************************************************************
     * @brief   引数解析
     * @arg		argc		(in)	引数の数
     * @arg		argv		(in)	引数配列
     * @return  なし
     * @note    
     *****************************************************************************/
    void vArgumentAnalyze(int argc, const char* argv[]) {
        for (int i = 0; i < argc; ++i) {
            std::string arg(argv[i]);
            auto pos = arg.find_last_of('=');
            if (pos == std::string::npos) {
                continue;
            }
            std::string key = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);
            m_mapArgument.emplace(key, value);
        }
        for (auto& itr : (m_mapArgument)) {
            std::string strArgKey = itr.first;
            std::string strArgVal = itr.second;
            std::stringstream ss;
            ss << "argument[";
            ss << std::setw(16) << std::right << strArgKey;
            ss << ":";
            ss << std::setw(32) << std::left << strArgVal;
            ss << ":";
            LCC_LOG_DEBUG(ss.str().c_str());
        }
    }

    /******************************************************************************
     * @brief   設定の読み込み
     * @arg     なし
     * @return  なし
     * @note    
     *****************************************************************************/
    void vLoadConfig() {
        uint64_t    unExpireSec(0);
        uint32_t    unLogMask(0xFFFFFFFF);
        std::string strLogFilePrefix;
        std::string strLogDir;

		bool bReadIniFileSuccess = false;
        // iniファイル読み込み
        if (m_cIniFile.LoadFromFile(m_strIniFile)) {
            unLogMask        = static_cast<uint32_t>(std::stoul(m_cIniFile.Get("Log", "Mask",          "0xFFFFFFFF"), nullptr, 0));
            unExpireSec      = std::stoull(m_cIniFile.Get(                     "Log", "ExpireSec",     "0"         ));
            strLogFilePrefix = m_cIniFile.Get(                                 "Log", "LogFilePrefix", "Log"       );
            strLogDir        = m_cIniFile.Get(                                 "Log", "LogDir",        "../log"    );
			bReadIniFileSuccess = true;
        }

        // Logger設定
        Logger::Instance().SetLogMask(unLogMask);
        Logger::Instance().SetFileExpireSeconds(unExpireSec);
        Logger::Instance().SetLogFilePrefix(strLogFilePrefix);
        Logger::Instance().SetLogDir(strLogDir);
        Logger::Instance().Start();

        if(bReadIniFileSuccess) {
            LCC_LOG_INFO("Config file [%s] loaded successfully.", m_strIniFile.c_str());
        }
        else {
            LCC_LOG_ALERT("Failed to load config file [%s]. Using default settings.", m_strIniFile.c_str());
		}
    }

    /******************************************************************************
     * @brief   MessageDrivenのRun関数を利用したメイン処理
     * @arg     なし
     * @return  int プロセス終了コード
     * @note    ブロッキング状態でメッセージを待ち、受信時に登録されたハンドラを呼び出す
     *****************************************************************************/
    void vRunProcess() {
        // MessageDriven::Run により、メッセージ受信を待つ
        Run([this]() { return m_bRunning.load(); }, 100);
        return;
    }

private:
    // メンバ変数
	std::atomic_bool  m_bRunning;              ///< プロセス稼働状態フラグ
    IniFile           m_cIniFile;              ///< iniファイルオブジェクト
    std::string       m_strIniFile;            ///< iniファイル名
    MessageHandlerMap m_mapMessageHandler;     ///< メッセージハンドラ群
    std::mutex        m_cMessageHandlerMutex;  ///< メッセージハンドラ保護用ミューテックス
    TimerHandlerMap   m_mapTimerHandler;       ///< タイマーハンドラ群
    std::mutex        m_cTimerHandlerMutex;    ///< タイマーハンドラ保護用ミューテックス
	SignalHandlerMap  m_mapSignalHandler;      ///< シグナルハンドラ群
	std::mutex        m_cSignalHandlerMutex;   ///< シグナルハンドラ保護用ミューテックス

    std::unordered_map<std::string, std::string> m_mapArgument;     ///< 引数マップ
	std::unique_ptr<TimerManager<ProcessEvent>>  m_pcTimerManager;  ///< タイマーマネージャ
};
}
