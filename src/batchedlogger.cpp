// Copyright (c) 2020 The BeeGroup developers are EternityGroup
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "batchedlogger.h"
#include "util.h"

CBatchedLogger::CBatchedLogger(const std::string& _category, const std::string& _header) :
    accept(LogAcceptCategory(_category.c_str())), header(_header)
{
}

CBatchedLogger::~CBatchedLogger()
{
    Flush();
}

void CBatchedLogger::Flush()
{
    if (!accept || msg.empty()) {
        return;
    }
    LogPrintStr(strprintf("%s:\n%s", header, msg));
    msg.clear();
}
