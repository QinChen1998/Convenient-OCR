#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <QWidget>
#include <QLabel>
#include <QRubberBand>
#include <QScreen>
#include <QApplication>
#include <QPixmap>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>

/**
 * @brief 屏幕截图选择器类
 * 提供全屏截图和区域选择功能
 */
class ScreenCapture : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenCapture(QWidget *parent = nullptr);
    ~ScreenCapture() = default;

    /**
     * @brief 开始截图选择
     * @return 成功启动返回true
     */
    bool startCapture();

signals:
    /**
     * @brief 截图完成信号
     * @param imagePath 保存的图片文件路径，空字符串表示用户取消
     */
    void captureFinished(const QString &imagePath);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    /**
     * @brief 保存选中区域的截图
     */
    void saveSelectedArea();

private:
    /**
     * @brief 初始化界面
     */
    void initUI();

    /**
     * @brief 截取全屏
     * @return 全屏截图
     */
    QPixmap captureFullScreen();

    /**
     * @brief 生成临时文件路径
     * @return 临时图片文件路径
     */
    QString generateTempFilePath();

private:
    QPixmap m_fullScreenPixmap;    // 全屏截图
    QRubberBand *m_rubberBand;     // 选择框
    QPoint m_startPoint;           // 选择起始点
    QPoint m_endPoint;             // 选择结束点
    bool m_isSelecting;            // 是否正在选择
    QRect m_selectedRect;          // 选中的矩形区域
};

#endif // SCREENCAPTURE_H