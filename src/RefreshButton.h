#pragma once

#include <QToolButton>

class QVariantAnimation;

// 메뉴 막대 오른쪽의 [⟳ 실시간 갱신] 버튼.
// 누르면 바로 시세를 다시 받아오고, 받는 동안 ⟳ 아이콘이 돌며, 다 받으면 한 번 반짝인다.
class RefreshButton : public QToolButton
{
    Q_OBJECT
public:
    explicit RefreshButton(QWidget *parent = nullptr);

    // 시세를 받는 중이면 true (아이콘 회전). false가 되면 지금 돌던 한 바퀴를 마치고 멈춘다.
    void setBusy(bool busy);
    // 갱신 완료 표시 (ok=false면 실패 색으로 반짝임)
    void flash(bool ok);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    QString label() const;

    QVariantAnimation *m_spin = nullptr;  // 아이콘 회전 (0 ~ 360도)
    QVariantAnimation *m_flash = nullptr; // 완료 반짝임 (1 → 0)
    qreal m_angle = 0.0;
    qreal m_glow = 0.0;
    bool m_flashOk = true;
    bool m_busy = false;
    bool m_hover = false;
};
