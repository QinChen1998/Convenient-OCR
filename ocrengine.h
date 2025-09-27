#ifndef OCRENGINE_H
#define OCRENGINE_H

#include <QString>
#include <QImage>
#include <QObject>

/**
 * @brief OCR引擎抽象基类
 *
 * 该类定义了OCR引擎的通用接口，所有具体的OCR实现都应该继承此类。
 * 这种设计模式允许在不修改现有代码的情况下轻松添加新的OCR引擎。
 */
class OCREngine : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief OCR引擎类型枚举
     */
    enum EngineType {
        TESSERACT,          // Tesseract OCR引擎
        PADDLE_OCR,         // PaddleOCR引擎（预留）
        EASY_OCR,           // EasyOCR引擎（预留）
        CUSTOM              // 自定义引擎（预留）
    };

    /**
     * @brief OCR识别结果结构体
     */
    struct OCRResult {
        QString text;           // 识别出的文本内容
        float confidence;       // 置信度（0.0-1.0）
        bool success;          // 是否识别成功
        QString errorMessage;   // 错误信息（如果失败）

        OCRResult() : confidence(0.0), success(false) {}
    };

    /**
     * @brief 批量OCR识别结果结构体
     */
    struct BatchOCRResult {
        QStringList texts;          // 每页识别出的文本内容列表
        QStringList pageNames;      // 页面名称列表
        QList<float> confidences;   // 每页置信度列表
        QString combinedText;       // 合并后的全部文本
        bool success;              // 是否处理成功
        QString errorMessage;       // 错误信息（如果失败）
        int processedPages;         // 成功处理的页面数
        int totalPages;             // 总页面数

        BatchOCRResult() : success(false), processedPages(0), totalPages(0) {}
    };

    explicit OCREngine(QObject *parent = nullptr);
    virtual ~OCREngine() = default;

    /**
     * @brief 获取引擎类型
     * @return 引擎类型枚举值
     */
    virtual EngineType getEngineType() const = 0;

    /**
     * @brief 获取引擎名称
     * @return 引擎显示名称
     */
    virtual QString getEngineName() const = 0;

    /**
     * @brief 初始化OCR引擎
     * @return 是否初始化成功
     */
    virtual bool initialize() = 0;

    /**
     * @brief 执行OCR识别
     * @param image 待识别的图像
     * @param language 识别语言代码（如"chi_sim", "eng"）
     * @return OCR识别结果
     */
    virtual OCRResult performOCR(const QImage &image, const QString &language = "chi_sim+eng") = 0;

    /**
     * @brief 执行批量OCR识别
     * @param images 待识别的图像列表
     * @param pageNames 页面名称列表
     * @param language 识别语言代码（如"chi_sim", "eng"）
     * @return 批量OCR识别结果
     */
    virtual BatchOCRResult performBatchOCR(const QList<QImage> &images,
                                          const QStringList &pageNames,
                                          const QString &language = "chi_sim+eng") = 0;

    /**
     * @brief 检查引擎是否可用
     * @return 是否可用
     */
    virtual bool isAvailable() const = 0;

    /**
     * @brief 获取支持的语言列表
     * @return 支持的语言代码列表
     */
    virtual QStringList getSupportedLanguages() const = 0;

signals:
    /**
     * @brief OCR处理进度信号
     * @param progress 进度百分比（0-100）
     */
    void progressUpdated(int progress);

    /**
     * @brief 批量OCR处理进度信号
     * @param progress 整体进度百分比（0-100）
     * @param currentPage 当前处理页面（从1开始）
     * @param totalPages 总页面数
     * @param currentPageProgress 当前页面进度（0-100）
     */
    void batchProgressUpdated(int progress, int currentPage, int totalPages, int currentPageProgress);

    /**
     * @brief OCR处理完成信号
     * @param result 识别结果
     */
    void ocrCompleted(const OCRResult &result);

    /**
     * @brief 批量OCR处理完成信号
     * @param result 批量识别结果
     */
    void batchOcrCompleted(const BatchOCRResult &result);

    /**
     * @brief 错误信号
     * @param errorMessage 错误信息
     */
    void errorOccurred(const QString &errorMessage);

protected:
    bool m_initialized;     // 引擎是否已初始化
    QString m_lastError;    // 最后的错误信息
};

#endif // OCRENGINE_H