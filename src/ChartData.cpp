#include "ChartData.h"

#include <QtMath>
#include <limits>

namespace Indicators {

namespace {
const double kNaN = std::numeric_limits<double>::quiet_NaN();

// NaN이 아닌 첫 위치부터 EMA 계산 (첫 값은 n개 단순평균)
QVector<double> emaFrom(const QVector<double> &v, int n)
{
    QVector<double> out(v.size(), kNaN);
    if (n <= 0)
        return out;
    int start = 0;
    while (start < v.size() && qIsNaN(v[start]))
        ++start;
    if (v.size() - start < n)
        return out;
    double sum = 0.0;
    for (int i = start; i < start + n; ++i)
        sum += v[i];
    double prev = sum / n;
    out[start + n - 1] = prev;
    const double k = 2.0 / (n + 1);
    for (int i = start + n; i < v.size(); ++i) {
        prev = v[i] * k + prev * (1.0 - k);
        out[i] = prev;
    }
    return out;
}
} // namespace

QVector<double> sma(const QVector<double> &v, int n)
{
    QVector<double> out(v.size(), kNaN);
    if (n <= 0)
        return out;
    double sum = 0.0;
    int valid = 0;
    for (int i = 0; i < v.size(); ++i) {
        if (qIsNaN(v[i])) {
            sum = 0.0;
            valid = 0;
            continue;
        }
        sum += v[i];
        ++valid;
        if (valid > n) {
            sum -= v[i - n];
            valid = n;
        }
        if (valid == n)
            out[i] = sum / n;
    }
    return out;
}

QVector<double> ema(const QVector<double> &v, int n)
{
    return emaFrom(v, n);
}

void macd(const QVector<double> &close, int fast, int slow, int signalN,
          QVector<double> &macdLine, QVector<double> &signalLine, QVector<double> &osc)
{
    const QVector<double> ef = emaFrom(close, fast);
    const QVector<double> es = emaFrom(close, slow);
    macdLine = QVector<double>(close.size(), kNaN);
    for (int i = 0; i < close.size(); ++i) {
        if (!qIsNaN(ef[i]) && !qIsNaN(es[i]))
            macdLine[i] = ef[i] - es[i];
    }
    signalLine = emaFrom(macdLine, signalN);
    osc = QVector<double>(close.size(), kNaN);
    for (int i = 0; i < close.size(); ++i) {
        if (!qIsNaN(macdLine[i]) && !qIsNaN(signalLine[i]))
            osc[i] = macdLine[i] - signalLine[i];
    }
}

QVector<double> rsi(const QVector<double> &close, int n)
{
    QVector<double> out(close.size(), kNaN);
    if (close.size() <= n || n <= 0)
        return out;
    double gain = 0.0, loss = 0.0;
    for (int i = 1; i <= n; ++i) {
        const double d = close[i] - close[i - 1];
        if (d > 0)
            gain += d;
        else
            loss -= d;
    }
    gain /= n;
    loss /= n;
    auto value = [](double g, double l) { return l == 0.0 ? (g == 0.0 ? 50.0 : 100.0) : 100.0 - 100.0 / (1.0 + g / l); };
    out[n] = value(gain, loss);
    for (int i = n + 1; i < close.size(); ++i) {
        const double d = close[i] - close[i - 1];
        gain = (gain * (n - 1) + (d > 0 ? d : 0.0)) / n;
        loss = (loss * (n - 1) + (d < 0 ? -d : 0.0)) / n;
        out[i] = value(gain, loss);
    }
    return out;
}

void stochasticSlow(const QVector<double> &high, const QVector<double> &low, const QVector<double> &close,
                    int n, int kPeriod, int dPeriod, QVector<double> &k, QVector<double> &d)
{
    QVector<double> fast(close.size(), kNaN);
    for (int i = n - 1; i < close.size(); ++i) {
        double hh = high[i], ll = low[i];
        for (int j = i - n + 1; j <= i; ++j) {
            hh = qMax(hh, high[j]);
            ll = qMin(ll, low[j]);
        }
        fast[i] = (hh - ll) > 0 ? (close[i] - ll) / (hh - ll) * 100.0 : 50.0;
    }
    k = sma(fast, kPeriod);
    d = sma(k, dPeriod);
}

void bollinger(const QVector<double> &close, int n, double mult,
               QVector<double> &mid, QVector<double> &upper, QVector<double> &lower)
{
    mid = sma(close, n);
    upper = QVector<double>(close.size(), kNaN);
    lower = QVector<double>(close.size(), kNaN);
    for (int i = n - 1; i < close.size(); ++i) {
        if (qIsNaN(mid[i]))
            continue;
        double var = 0.0;
        for (int j = i - n + 1; j <= i; ++j)
            var += (close[j] - mid[i]) * (close[j] - mid[i]);
        const double sd = qSqrt(var / n);
        upper[i] = mid[i] + mult * sd;
        lower[i] = mid[i] - mult * sd;
    }
}

} // namespace Indicators
