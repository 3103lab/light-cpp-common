// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    MessageDriven.h
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
    template<typename TMessage>
    class MessageDriven
    {
    public:
        MessageDriven() = default;
        virtual ~MessageDriven() = default;

        /******************************************************************************
         * @brief   メッセージ送信
         * @param   msg (in) ポストするメッセージ
         * @return  結果
         * @retval  true:成功 false:停止済み
         * @note
         *****************************************************************************/
        bool Post(const TMessage& msg) {
            return m_queMessage.Enq(msg);
        }

        /******************************************************************************
         * @brief   メッセージ処理ループ（呼び出し側スレッドで使用）
         * @param   bContinue (in) 処理継続判定
         * @param   unTimeout (in) 1回の待機時間（ms）
         * @return  なし
         * @retval  なし
         * @note
         *****************************************************************************/
        void Run(const std::function<bool()>& bContinue, uint64_t unTimeout = 0) {
            TMessage msg;
            while (bContinue()) {
                if (m_queMessage.Deq(msg, unTimeout)) {
                    try {
                        OnMessage(msg);
                    } catch (const std::exception& ex) {
                        LogOnMessageException(ex);
                    } catch (...) {
                        LogOnMessageException();
                    }
                }
            }
        }

        void Shutdown() { m_queMessage.Shutdown(); }
        bool IsShutdown() const { return m_queMessage.IsShutdown(); }

    protected:
        virtual void OnMessage(const TMessage& msg) = 0;


        /******************************************************************************
         * @brief   Exception発生時のロギング
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note    Loggerが本クラスに依存するため、
         *          循環参照になるためLoggerではロギングできない
         *          仮想関数にしておくので、Loggerを使う場合はオーバーライドすること
         *****************************************************************************/
        virtual void LogOnMessageException(const std::exception& ex) {
            std::cerr << "Exception in OnMessage: " << ex.what() << std::endl;
            // LOG_ERROR("Exception in OnMessage(): %s", ex.what());
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
        virtual void LogOnMessageException() {
            std::cerr << "Unknown Exception in OnMessage" << std::endl;
            // LOG_ERROR("Unknown Exception in OnMessage(): %s");
        }

    private:
        LockedQueue<TMessage> m_queMessage;
    };
}