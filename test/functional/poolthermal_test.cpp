#include "qt/poolthermalpolicy.h"

#include <cassert>
#include <limits>

int main()
{
    using namespace poolthermal;
    Policy cpu, gpu;
    const auto missing = std::numeric_limits<double>::quiet_NaN();
    assert(cpu.Sample(missing, 8, false).threads == 0);
    const auto low = cpu.Sample(missing, 8, true);
    assert(low.state == State::LowLoad && low.threads == 1 && low.delayMs >= 250);
    assert(cpu.Sample(-10, 8, false).threads == 8);
    assert(cpu.Sample(-3, 8, false).threads == 4);
    assert(cpu.Sample(0, 8, false).state == State::Cooling);
    assert(gpu.Sample(-10, 1, false).state == State::Normal);
    assert(cpu.Sample(missing, 8, true).threads == 0);
    for (int i = 0; i < 4; ++i) assert(cpu.Sample(-6, 8, false).threads == 0);
    assert(cpu.Sample(-1, 8, false).threads == 0);
    for (int i = 0; i < 4; ++i) assert(cpu.Sample(-6, 8, false).threads == 0);
    assert(cpu.Sample(-6, 8, false).threads == 2);
    assert(cpu.Sample(-6, 8, false).threads == 4);
    assert(cpu.Sample(-6, 8, false).threads == 6);
    assert(cpu.Sample(-6, 8, false).threads == 8);
}
