// Standalone tests for the first-party Stratum message allowlist.
#include "qt/poolframes.h"
#include "qt/pooltemperatures.h"

#include <cassert>
#include <cstdio>

int main()
{
    using poolframes::Classify;
    using poolframes::Kind;
    assert(Classify(R"({"id":1,"result":true,"error":null})") == Kind::Reply);
    assert(Classify(R"({"id":1.5,"result":true})") == Kind::Invalid);
    assert(Classify(R"({"id":"1","result":true})") == Kind::Invalid);
    assert(Classify(R"({"id":null,"method":"mining.notify","params":[]})") == Kind::Work);
    assert(Classify(R"({"method":"mining.set_difficulty","params":[1]})") == Kind::Difficulty);
    assert(Classify(R"({"method":"mining.set_extranonce","params":["00000000",4]})") == Kind::ExtraNonce);
    assert(Classify(R"({"method":"client.reconnect","params":["other.invalid",9,0]})") == Kind::Ignore);
    assert(Classify(R"({"method":"client.get_stats","params":[]})") == Kind::Ignore);
    assert(Classify(R"({"method":"mining.notify","result":true,"params":[]})") == Kind::Invalid);
    assert(Classify(R"({"method":"mining.notify","params":{}})") == Kind::Invalid);
    assert(Classify("[]") == Kind::Invalid);
    assert(Classify("{}\n{}") == Kind::Invalid);
    assert(Classify(QByteArray("{}\0", 3)) == Kind::Invalid);
    assert(Classify(QByteArray(poolframes::kMaxFrameBytes + 1, ' ')) == Kind::Invalid);
    const auto direct = poolthermal::ParseWindowsReading("CPU 72.5\r\n", false);
    assert(direct.core == 72.5 && !direct.proxy && direct.fresh);
    const auto proxy = poolthermal::ParseWindowsReading("PROXY 43.9\r\n", true);
    assert(proxy.core == 43.9 && proxy.proxy);
    assert(!std::isfinite(poolthermal::ParseWindowsReading("PROXY 43.9", false).core));
    assert(!std::isfinite(poolthermal::ParseWindowsReading("CPU 273.15", true).core));
    assert(!std::isfinite(poolthermal::ParseWindowsReading("CPU 54\nPROXY 40", true).core));
    assert(!std::isfinite(poolthermal::ParseWindowsReading("CPU NAN", true).core));
    std::puts("First-party Stratum framing: allowlist, redirect rejection, ambiguity and input limits passed.");
}
