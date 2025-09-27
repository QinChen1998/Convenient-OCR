#ifndef TESSERACTOCRENGINE_H
#define TESSERACTOCRENGINE_H

#include "ocrengine.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>

/**
 * @brief Tesseract OCR引擎实现类
 *
 * 该类实现了基于Tesseract OCR的文字识别功能。
 * Tesseract是Google开源的OCR引擎，支持多种语言识别。
 */
class TesseractOCREngine : public OCREngine
{
    Q_OBJECT

public:
    explicit TesseractOCREngine(QObject *parent = nullptr);
    ~TesseractOCREngine() override;

    // 重写基类的虚函数
    EngineType getEngineType() const override;
    QString getEngineName() const override;
    bool initialize() override;
    OCRResult performOCR(const QImage &image, const QString &language = "chi_sim+eng") override;
    BatchOCRResult performBatchOCR(const QList<QImage> &images,
                                   const QStringList &pageNames,
                                   const QString &language = "chi_sim+eng") override;
    bool isAvailable() const override;
    QStringList getSupportedLanguages() const override;

    /**
     * @brief 设置Tesseract可执行文件路径
     * @param path Tesseract可执行文件的完整路径
     */
    void setTesseractPath(const QString &path);

    /**
     * @brief 设置Tesseract数据目录
     * @param path tessdata目录路径
     */
    void setTessDataPath(const QString &path);

    /**
     * @brief 设置OCR引擎模式
     * @param mode OCR引擎模式 (0-13)
     */
    void setOCREngineMode(int mode);

    /**
     * @brief 设置页面分割模式
     * @param mode 页面分割模式 (0-13)
     */
    void setPageSegmentationMode(int mode);

private slots:
    /**
     * @brief 处理Tesseract进程完成信号
     */
    void onTesseractFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /**
     * @brief 处理Tesseract进程错误信号
     */
    void onTesseractError(QProcess::ProcessError error);

private:
    /**
     * @brief 检查Tesseract是否已安装
     * @return 是否安装
     */
    bool checkTesseractInstallation() const;

    /**
     * @brief 获取Tesseract版本信息
     * @return 版本字符串
     */
    QString getTesseractVersion();

    /**
     * @brief 保存图像到临时文件
     * @param image 要保存的图像
     * @return 临时文件路径，失败返回空字符串
     */
    QString saveImageToTempFile(const QImage &image);

    /**
     * @brief 从文件读取OCR结果
     * @param filePath 结果文件路径
     * @return 读取到的文本内容
     */
    QString readOCRResultFromFile(const QString &filePath);

    /**
     * @brief 清理临时文件
     */
    void cleanupTempFiles();

    /**
     * @brief 从TSV文件解析置信度信息
     * @param tsvFilePath TSV文件路径
     * @return 平均置信度（0.0-1.0）
     */
    float parseConfidenceFromTSV(const QString &tsvFilePath);

    /**
     * @brief 执行单页OCR识别（专用于批量处理，包含批量进度更新）
     * @param image 待识别的图像
     * @param language 识别语言代码
     * @param pageIndex 当前页面索引
     * @param totalPages 总页面数
     * @return OCR识别结果
     */
    OCRResult performSinglePageOCRWithBatchProgress(const QImage &image,
                                                   const QString &language,
                                                   int pageIndex,
                                                   int totalPages);

private:
    QString m_tesseractPath;        // Tesseract可执行文件路径
    QString m_tessDataPath;         // tessdata数据目录路径
    int m_ocrEngineMode;           // OCR引擎模式
    int m_pageSegmentationMode;    // 页面分割模式
    QProcess *m_tesseractProcess;  // Tesseract进程对象
    QStringList m_tempFiles;       // 临时文件列表，用于清理
    OCRResult m_currentResult;     // 当前OCR结果（用于异步处理）
    bool m_processingAsync;        // 是否正在异步处理

    // 常用语言代码映射
    static const QMap<QString, QString> s_languageMap;
};

#endif // TESSERACTOCRENGINE_H