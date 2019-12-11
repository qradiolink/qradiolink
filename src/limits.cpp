#include "limits.h"

Limits::Limits()
{
    // TODO: configurable band allocation
    _tx_limits.push_back(std::complex<long long>(1810000, 2000000));
    _tx_limits.push_back(std::complex<long long>(3500000, 3800000));
    _tx_limits.push_back(std::complex<long long>(7000000, 7200000));
    _tx_limits.push_back(std::complex<long long>(10100000, 10150000));
    _tx_limits.push_back(std::complex<long long>(10100000, 10150000));
    _tx_limits.push_back(std::complex<long long>(14000000, 14350000));
    _tx_limits.push_back(std::complex<long long>(18068000, 18168000));
    _tx_limits.push_back(std::complex<long long>(21000000, 21450000));
    _tx_limits.push_back(std::complex<long long>(24890000, 24990000));
    _tx_limits.push_back(std::complex<long long>(28000000, 29700000));
    _tx_limits.push_back(std::complex<long long>(50000000, 52000000));
    _tx_limits.push_back(std::complex<long long>(70000000, 70300000));
    _tx_limits.push_back(std::complex<long long>(144000000, 146000000));
    _tx_limits.push_back(std::complex<long long>(430000000, 440000000));
    _tx_limits.push_back(std::complex<long long>(1240000000, 1300000000));
    _tx_limits.push_back(std::complex<long long>(2300000000, 2450000000));
    _tx_limits.push_back(std::complex<long long>(2300000000, 2450000000));
    _tx_limits.push_back(std::complex<long long>(3400000000, 3410000000));
    _tx_limits.push_back(std::complex<long long>(5660000000, 5670000000));
    _tx_limits.push_back(std::complex<long long>(5725000000, 5850000000));
    _tx_limits.push_back(std::complex<long long>(5725000000, 5850000000));
    _tx_limits.push_back(std::complex<long long>(10000000000, 10300000000));
}

bool Limits::checkLimit(double freq)
{
    long long tx_freq = (long long)freq;
    for(unsigned int i=0;i<_tx_limits.size();i++)
    {
        std::complex<long long> band = _tx_limits.at(i);
        if(tx_freq > band.real() && tx_freq < band.imag())
            return true;
    }
    return false;
}
