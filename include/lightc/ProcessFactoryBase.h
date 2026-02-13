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

#include <memory>
#include <cstdlib>
#include <lightc/ProcessBase.h>

namespace LCC
{
class ProcessFactoryBase {
public:
    virtual ~ProcessFactoryBase() {}
    virtual std::shared_ptr<ProcessBase> CreateApplicationProcess() = 0;
};
extern std::shared_ptr<ProcessFactoryBase> GetProcessFactory();
}

/******************************************************************************
 * @brief	エントリーポイント
 * @arg		argc		(in)	引数の数
 * @arg		argv		(in)	引数配列
 * @return	なし
 * @retval	なし
 * @note
 ******************************************************************************
 */
int main(int argc, const char* argv[])
{
    std::shared_ptr<LCC::ProcessFactoryBase> pcProcessFactory = LCC::GetProcessFactory();
    std::shared_ptr<LCC::ProcessBase> pcProcess = pcProcessFactory->CreateApplicationProcess();
    pcProcess->Initialize(argc, argv);
    pcProcess->Start();
    return 0;
}
