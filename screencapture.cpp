#include "screencapture.h"
#include <QGuiApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QKeyEvent>

ScreenCapture::ScreenCapture(QWidget *parent)
    : QWidget(parent)
    , m_rubberBand(nullptr)
    , m_isSelecting(false)
{
    initUI();
}

bool ScreenCapture::startCapture()
{
    // 截取全屏
    m_fullScreenPixmap = captureFullScreen();
    if (m_fullScreenPixmap.isNull()) {
        emit captureFinished("");
        return false;
    }

    // 设置窗口全屏显示
    setWindowState(Qt::WindowFullScreen);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    setCursor(Qt::CrossCursor);

    // 显示窗口
    show();
    activateWindow();
    raise();

    return true;
}

void ScreenCapture::initUI()
{
    // 设置窗口属性
    setWindowTitle("屏幕截图选择");
    setAttribute(Qt::WA_DeleteOnClose);

    // 创建选择框
    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    m_rubberBand->hide();

    // 设置鼠标追踪
    setMouseTracking(true);
}

QPixmap ScreenCapture::captureFullScreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return QPixmap();
    }

    // 获取屏幕截图，并确保考虑设备像素比
    QPixmap pixmap = screen->grabWindow(0);
    pixmap.setDevicePixelRatio(screen->devicePixelRatio());

    return pixmap;
}

QString ScreenCapture::generateTempFilePath()
{
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir dir(tempDir);
    if (!dir.exists()) {
        dir.mkpath(tempDir);
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    return dir.absoluteFilePath(QString("ocr_screenshot_%1.png").arg(timestamp));
}

void ScreenCapture::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_startPoint = event->pos();
        m_endPoint = event->pos();
        m_isSelecting = true;

        if (m_rubberBand) {
            m_rubberBand->setGeometry(QRect(m_startPoint, m_endPoint).normalized());
            m_rubberBand->show();
        }
    }
    QWidget::mousePressEvent(event);
}

void ScreenCapture::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isSelecting && m_rubberBand) {
        m_endPoint = event->pos();
        QRect rect = QRect(m_startPoint, m_endPoint).normalized();
        m_rubberBand->setGeometry(rect);
        m_selectedRect = rect;
    }
    QWidget::mouseMoveEvent(event);
}

void ScreenCapture::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isSelecting) {
        m_isSelecting = false;
        m_endPoint = event->pos();
        m_selectedRect = QRect(m_startPoint, m_endPoint).normalized();

        // 检查选择区域的有效性
        if (m_selectedRect.width() > 10 && m_selectedRect.height() > 10) {
            saveSelectedArea();
        } else {
            // 选择区域太小，取消截图
            emit captureFinished("");
            close();
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void ScreenCapture::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (m_fullScreenPixmap.isNull()) {
        return;
    }

    QPainter painter(this);

    // 绘制全屏背景，考虑设备像素比
    QScreen *screen = QGuiApplication::primaryScreen();
    qreal devicePixelRatio = screen ? screen->devicePixelRatio() : 1.0;

    // 按逻辑尺寸绘制截图
    QRect targetRect = rect();
    painter.drawPixmap(targetRect, m_fullScreenPixmap);

    // 如果有选中区域，绘制半透明遮罩
    if (m_isSelecting || !m_selectedRect.isEmpty()) {
        // 绘制半透明遮罩
        painter.fillRect(rect(), QColor(0, 0, 0, 100));

        // 清除选中区域的遮罩，显示原图
        if (!m_selectedRect.isEmpty()) {
            painter.setCompositionMode(QPainter::CompositionMode_Clear);
            painter.fillRect(m_selectedRect, Qt::transparent);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

            // 重新绘制选中区域的原图
            // 计算物理坐标区域
            QRect physicalRect = QRect(
                m_selectedRect.x() * devicePixelRatio,
                m_selectedRect.y() * devicePixelRatio,
                m_selectedRect.width() * devicePixelRatio,
                m_selectedRect.height() * devicePixelRatio
            );
            painter.drawPixmap(m_selectedRect, m_fullScreenPixmap, physicalRect);
        }
    }
}

void ScreenCapture::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        // ESC键取消截图
        emit captureFinished("");
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ScreenCapture::saveSelectedArea()
{
    if (m_selectedRect.isEmpty() || m_fullScreenPixmap.isNull()) {
        emit captureFinished("");
        close();
        return;
    }

    // 获取设备像素比
    QScreen *screen = QGuiApplication::primaryScreen();
    qreal devicePixelRatio = screen ? screen->devicePixelRatio() : 1.0;

    // 将逻辑坐标转换为物理坐标
    QRect physicalRect = QRect(
        m_selectedRect.x() * devicePixelRatio,
        m_selectedRect.y() * devicePixelRatio,
        m_selectedRect.width() * devicePixelRatio,
        m_selectedRect.height() * devicePixelRatio
    );

    // 从全屏截图中提取选中区域（使用物理坐标）
    QPixmap selectedPixmap = m_fullScreenPixmap.copy(physicalRect);

    // 设置正确的设备像素比，确保显示尺寸正确
    selectedPixmap.setDevicePixelRatio(devicePixelRatio);

    // 生成临时文件路径
    QString filePath = generateTempFilePath();

    // 保存图片
    if (selectedPixmap.save(filePath, "PNG")) {
        emit captureFinished(filePath);
    } else {
        emit captureFinished("");
    }

    close();
}