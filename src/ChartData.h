#pragma once

#include <QDate>
#include <QVector>

// 캔들(봉) 한 개
struct Candle
{
    QDate date;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double volume = 0.0;
};

// 보조지표 계산. 값이 없는 구간은 NaN.
namespace Indicators {

QVector<double> sma(const QVector<double> &v, int n);
QVector<double> ema(const QVector<double> &v, int n);

// MACD(fast, slow, signal): macd = EMA(fast) - EMA(slow), signal = EMA(macd), osc = macd - signal
void macd(const QVector<double> &close, int fast, int slow, int signalN,
          QVector<double> &macdLine, QVector<double> &signalLine, QVector<double> &osc);

// RSI (Wilder 방식)
QVector<double> rsi(const QVector<double> &close, int n);

// 스토캐스틱 슬로우(n, k, d)
void stochasticSlow(const QVector<double> &high, const QVector<double> &low, const QVector<double> &close,
                    int n, int kPeriod, int dPeriod, QVector<double> &k, QVector<double> &d);

// 볼린저 밴드(n, 배수)
void bollinger(const QVector<double> &close, int n, double mult,
               QVector<double> &mid, QVector<double> &upper, QVector<double> &lower);

} // namespace Indicators
