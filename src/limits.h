#ifndef LIMITS_H
#define LIMITS_H

#include <vector>
#include <complex>

class Limits
{
public:
    Limits();
    bool checkLimit(long long tx_freq);

private:
    std::vector<std::complex<long long>> _tx_limits;
};

#endif // LIMITS_H
