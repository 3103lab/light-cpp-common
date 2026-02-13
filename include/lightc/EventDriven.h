// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    EventDriven.h
 * @brief   C++/Common Library Message Driven Process
 * @author  Hikari Satoh
 * @note    
 *  
 * Copyright (c) 2025 Hikari Satoh
 *               https://3103lab.com
 ******************************************************************************
 */
#pragma once
#include "LockedQueue.h"
#include <iostream>
#include <functional>

namespace LCC
{
    template<typename TEvent_>
    class EventDriven
    {
    public:
        EventDriven() = default;
        virtual ~EventDriven() = default;

        /******************************************************************************
         * @brief   メッセージ送信
         * @param   msg (in) ポストするメッセージ
         * @return  結果
         * @retval  true:成功 false:停止済み
         * @note
         *****************************************************************************/
        bool Post(const TEvent_& msg) {
            return m_queEvent.Enq(msg);
        }

        /******************************************************************************
         * @brief   メッセージ処理ループ（呼び出し側スレッドで使用）
         * @param   bContinue (in) 処理継続判定
         * @param   unTimeout (in) 1回の待機時間（ms）
         * @return  なし
         * @retval  なし
         * @note
         *****************************************************************************/
        void Run(const std::function<bool()>& fnContinue, uint64_t unTimeout = 0) {
            TEvent_ msg;
            while (fnContinue()) {
                if (m_queEvent.Deq(msg, unTimeout)) {
                    try {
                        vOnEvent(msg);
                    } catch (const std::exception& ex) {
                        LogOnEventException(ex);
                    } catch (...) {
                        LogOnEventException();
                    }
                }
            }
        }

        void Shutdown() { m_queEvent.Shutdown(); }
        bool IsShutdown() const { return m_queEvent.IsShutdown(); }

    protected:
        virtual void vOnEvent(const TEvent_& msg) = 0;


        /******************************************************************************
         * @brief   Exception発生時のロギング
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    Loggerが本クラスに依存するため、
         *          循環参照になるためLoggerではロギングできない
         *          仮想関数にしておくので、Loggerを使う場合はオーバーライドすること
         *****************************************************************************/
        virtual void LogOnEventException(const std::exception& ex) {
            std::cerr << "Exception in OnEvent: " << ex.what() << std::endl;
            // LOG_ERROR("Exception in OnEvent(): %s", ex.what());
        }

        /******************************************************************************
         * @brief   Exception発生時のロギング
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    Loggerが本クラスに依存するため、
         *          循環参照になるためLoggerではロギングできない
         *          仮想関数にしておくので、Loggerを使う場合はオーバーライドすること
         *****************************************************************************/
        virtual void LogOnEventException() {
            std::cerr << "Unknown Exception in OnEvent" << std::endl;
            // LOG_ERROR("Unknown Exception in OnEvent(): %s");
        }

    private:
        LockedQueue<TEvent_> m_queEvent;
    };
}