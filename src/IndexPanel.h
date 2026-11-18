#pragma once

#include "StockService.h"

#include <QWidget>

// 코스피/코스닥 지수를 크게 표시하는 패널.
// 창 크기에 맞춰 글자 크기가 자동으로 조절되도록 직접 그린다.
class IndexPanel : public QWidget
{
    Q_OBJECT
public:
    IndexPanel(const QString &code, const QString &title, QWidget *parent = nullptr);

    QString code() const { return m_code; }
    void setQuote(const Quote &q);
    void setStale(bool stale);

    QSize sizeHint() const override { return {280, 170}; }
    QSize minimumSizeHint() const override { return {160, 130}; }

signals:
    void doubleClicked(const QString &code);

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

private:
    QString m_code;
    QString m_title;
    Quote m_quote;
    bool m_hasQuote = false;
    bool m_stale = false;
};
