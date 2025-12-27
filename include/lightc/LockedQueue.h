// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    LockedQueue.h
 * @brief   Queue with Lock
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

#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>


namespace LCC
{
    template<class T_>
    class LockedQueue
    {
    public:
        /******************************************************************************
         * @brief   コンストラクタ
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    
         *****************************************************************************
         */
        LockedQueue()
            : m_bShutdown(false)
        {
        }

        /******************************************************************************
         * @brief   デストラクタ
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    終了処理としてShutdown()を呼ぶ
         *****************************************************************************
         */
        virtual ~LockedQueue() {
            Shutdown();
        }
        
        LockedQueue(const LockedQueue&) = delete;
        LockedQueue& operator=(const LockedQueue&) = delete;
        LockedQueue(LockedQueue&&) = delete;
        LockedQueue& operator=(LockedQueue&&) = delete;

    public:
        /******************************************************************************
         * @brief   エンキュー
         * @param   pcData  (in)    エンキューするデータ
         * @return  結果
         * @retval  true:正常 false:シャットダウン中
         * @note    シャットダウン後は受け付けない
         *****************************************************************************
         */
        bool Enq(const T_& pcData)
        {
            std::unique_lock<std::mutex> pcLock(m_mutexQue);
            if (m_bShutdown) return false;
            m_que.push(pcData);
            m_cvQue.notify_one();
            return true;
        }
        
        /******************************************************************************
         * @brief   デキュー
         * @param   pcData     (out)   デキューしたデータ
         * @param   unTimeout  (in)    タイムアウト時間（ミリ秒）省略時は無限待ち
         * @return  結果
         * @retval  true:正常取得 false:タイムアウトまたはシャットダウン
         * @note    タイムアウト0で無限待ちになる
         *****************************************************************************
         */
        bool Deq(T_& pcData, uint64_t unTimeout = 0)
        {
            std::unique_lock<std::mutex> pcLock(m_mutexQue);
            bool bReady = false;

            if (unTimeout == 0) {
                // 無限待ち
                m_cvQue.wait(pcLock, [this] {
                    return !m_que.empty() || m_bShutdown;
                });
                bReady = true; // waitが解除された時点でbReadyをtrueにする
            } else {
                // 指定時間待ち
                std::chrono::milliseconds cTimeout(unTimeout);
                bReady = m_cvQue.wait_for(pcLock, cTimeout, [this] {
                    return !m_que.empty() || m_bShutdown;
                });
            }

            if (!bReady || (m_bShutdown && m_que.empty())) {
                return false;
            }

            pcData = m_que.front();
            m_que.pop();
            return true;
        }
        
        /******************************************************************************
         * @brief   サイズ取得
         * @param   なし
         * @return  キューのサイズ
         * @retval  size_t
         * @note
         *****************************************************************************
         */
        size_t Size()
        {
            std::unique_lock<std::mutex> lock(m_mutexQue);
            return m_que.size();
        }

        /******************************************************************************
         * @brief   シャットダウン
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    Deq()の待機を解除する
         *****************************************************************************
         */
        void Shutdown()
        {
            std::unique_lock<std::mutex> lock(m_mutexQue);
            m_bShutdown = true;
            m_cvQue.notify_all();
        }

        /******************************************************************************
         * @brief   シャットダウン状態確認
         * @param   なし
         * @return  結果
         * @retval  true:終了指示済 false:稼働中
         * @note
         *****************************************************************************
         */
        bool IsShutdown() const {
            return m_bShutdown;
        }

    private:
        std::queue<T_>              m_que;
        std::mutex                  m_mutexQue;
        std::condition_variable     m_cvQue;
        std::atomic_bool            m_bShutdown;
    };
}