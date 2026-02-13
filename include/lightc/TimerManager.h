#pragma once
/******************************************************************************
 * @file    TimerManager.h
 * @brief   C++/Common Library Timer Manager Class
 * @author  Satoh  
 * @note    
 *  
 * Copyright (c) 2024 Satoh(3103lab.com)
 *****************************************************************************/
 
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <lightc/EventDriven.h>

namespace LCC
{
using TimerId = uint64_t;
template<typename TEvent_>
class TimerManager {
public:
    TimerManager(const std::weak_ptr<EventDriven<TEvent_>>& wpReceiver)
        : m_wpReceiver(wpReceiver), m_bShutdown(false) {}

    ~TimerManager() {
        StopAllTimers();
    }

    /******************************************************************************
     * @brief   タイマー登録関数（ワンショット）
     * @arg     unTimerId (in) タイマーID（ユニーク識別子）
     * @arg     unDelayMs (in) 遅延時間（ミリ秒）
     * @arg     pcMsg     (in) 送信するメッセージ
     * @return  なし
     * @retval  なし
     * @note    同一IDが登録されていた場合は置き換える
     *****************************************************************************/
    void StartTimer(TimerId unTimerId, uint64_t unDelayMs, std::unique_ptr<TEvent_> pcMsg) {
        StopTimer(unTimerId);  // 既存タイマーを停止

        std::thread th([this, unTimerId, unDelayMs, pcMsg = std::move(pcMsg)]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(unDelayMs));

            if (m_bShutdown.load()) return;

            auto spReceiver = m_wpReceiver.lock();
            if (spReceiver) {
                spReceiver->Post(*pcMsg);
            }

            vRemoveTimer(unTimerId);
        });

        std::lock_guard<std::mutex> lock(m_mtxTimers);
        m_mapTimers[unTimerId] = std::move(th);
    }

    /******************************************************************************
     * @brief   タイマーの停止（ID指定）
     * @arg     unTimerId (in) 停止対象のタイマーID
     * @return  なし
     * @retval  なし
     * @note    登録中のタイマーを途中で破棄する
     *****************************************************************************/
    void StopTimer(TimerId unTimerId) {
        std::lock_guard<std::mutex> lock(m_mtxTimers);
        auto it = m_mapTimers.find(unTimerId);
        if (it != m_mapTimers.end()) {
            if (it->second.joinable()) {
                it->second.detach();  // 遅延中でも即破棄（Postしない保証はない）
            }
            m_mapTimers.erase(it);
        }
    }

    /******************************************************************************
     * @brief   登録中の全タイマーを停止する
     * @arg     なし
     * @return  なし
     * @retval  なし
     * @note    アプリ終了時に必ず呼び出すこと
     *****************************************************************************/
    void StopAllTimers() {
        m_bShutdown.store(true);

        std::lock_guard<std::mutex> lock(m_mtxTimers);
        for (auto& [unTimerId, th] : m_mapTimers) {
            if (th.joinable()) {
                th.detach();  // 安全に終了しないが即座に破棄
            }
        }
        m_mapTimers.clear();
    }

private:
    /******************************************************************************
     * @brief   登録済みタイマーの削除（内部関数）
     * @arg     unTimerId (in) 削除対象のタイマーID
     * @return  なし
     * @retval  なし
     * @note    タイマー終了後に呼ばれる
     *****************************************************************************/
    void vRemoveTimer(TimerId unTimerId) {
        std::lock_guard<std::mutex> lock(m_mtxTimers);
        auto itr = m_mapTimers.find(unTimerId);
        if (itr != m_mapTimers.end()) {
            itr->second.detach();
            m_mapTimers.erase(unTimerId);
        }
    }

private:
    std::weak_ptr<EventDriven<TEvent_>> m_wpReceiver;
    std::atomic<bool> m_bShutdown;
    std::unordered_map<TimerId, std::thread> m_mapTimers;
    std::mutex m_mtxTimers;
};
}