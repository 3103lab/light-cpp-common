// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    Signal.h
 * @brief   Signal Wait Function
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
#include <csignal>
#include <initializer_list>

#include <atomic>
#include <thread>
#include <chrono>

#ifndef SIGUSR2
#define SIGUSR2 10002
#endif

namespace LCC::Signal
{

inline std::atomic<int>  g_snSignal{0};
inline std::atomic<bool> g_bWaitInUse{ false };

/******************************************************************************
    * @brief   シグナルハンドラ
    * @param   なし
    * @return  なし
    * @retval  なし
    * @note
    *****************************************************************************
    */
inline void vSignalHandler(int snSig)
{
    g_snSignal.store(snSig, std::memory_order_relaxed);
}

/******************************************************************************
 * @brief   シグナル発生
 * @param   snSig   (in)    発生させるシグナル番号
 * @return  なし
 * @retval  なし
 * @note
 *****************************************************************************
 */
inline void Raise(int64_t snSig)
{
    if (snSig == 0) return;
    g_snSignal.store(static_cast<int>(snSig), std::memory_order_relaxed);
}

/******************************************************************************
 * @brief   シグナル待機
 * @param   listSignal        (in)    待機するシグナル番号リスト
 * @param   unCheckIntervalMs (in)    シグナルチェック間隔（ミリ秒）
 * @return  シグナル番号
 * @retval  なし
 * @note    シグナル待機中は他のWait呼び出しを拒否する
 * @throw   std::logic_error 他スレッドでWait中に呼ばれた場合
 *****************************************************************************
 */
inline int64_t Wait(std::list<int64_t> listSignal, uint64_t unCheckIntervalMs = 200)
{
    bool expected = false;
    if (!g_bWaitInUse.compare_exchange_strong(expected, true)) {
        throw std::logic_error("LCC::Signal::Wait called while another Wait is in progress");
    }

    for (int64_t snSignal : listSignal) {
        std::signal(static_cast<int>(snSignal), vSignalHandler);
    }

    while (true) {
        int64_t snSignal = static_cast<int64_t>( g_snSignal.exchange(0, std::memory_order_relaxed) );
        if (snSignal != 0){
            g_bWaitInUse.store(false);
            return snSignal;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(unCheckIntervalMs));

    }
    g_bWaitInUse.store(false);
}

} // namespace LCC::Signal