// SPDX-License-Identifier: MIT
/******************************************************************************
 * @file    ProcessEvent.h
 * @brief   Process Event Define
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
#include <memory>
#include <vector>
#include <variant>
#include <lightc/TimerManager.h>

namespace LCC
{
	using Payload = std::vector<uint8_t>;
    using SignalNo = int64_t;
    struct MessageEvent
    {
        std::string                    strEventName; // ルーティング用
        std::shared_ptr<const Payload> spPayload;    // ペイロード本体
    };

    struct TimerEvent
    {
        TimerId                        unTimerId;    // ルーティング用
    };

    struct SignalEvent {
        SignalNo                       snSignal;     // ルーティング用(SIGTERM, SIGINT...)
    };

    using ProcessEvent = std::variant<MessageEvent, TimerEvent, SignalEvent>;
}