// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    MessageDriven.h
 * @brief   Message Driven WorkerThread Base Class
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
#include <thread>
#include <atomic>
#include <memory>

namespace LCC
{
    template<typename TMessage>
    class WorkerThreadBase
    {
    public:
        explicit WorkerThreadBase(std::shared_ptr<EventDriven<TMessage>> spMessageDriven)
            : m_spMessageDriven(std::move(spMessageDriven)), m_bRunning(false)
        {
        }

        virtual ~WorkerThreadBase() {
            Stop();
        }

        /******************************************************************************
         * @brief   スレッド開始
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note
         *****************************************************************************
         */
        void Start() {
            if (m_bRunning || !m_spMessageDriven) return;
            m_bRunning.store(true);

            m_thread = std::thread([this]() {
                m_spMessageDriven->Run([this]() {
                    return m_bRunning.load();
                }, 100);
            });
        }

        /******************************************************************************
         * @brief   スレッド停止
         * @param   なし
         * @return  なし
         * @retval  なし
         * @note
         *****************************************************************************
         */
        void Stop() {
            m_bRunning.store(false);

            if (m_spMessageDriven) {
                m_spMessageDriven->Shutdown();
            }

            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

    private:
        std::shared_ptr<EventDriven<TMessage>> m_spMessageDriven;
        std::atomic_bool m_bRunning;
        std::thread m_thread;
    };
}