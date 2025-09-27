#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QObject>
#include <QImage>
#include <QStringList>
#include <QFileInfo>

/**
 * @brief 文件处理器类
 *
 * 该类负责处理各种文件格式，将它们转换为可供OCR引擎识别的QImage对象。
 * 支持常见的图像格式和PDF文档格式。
 */
class FileProcessor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 支持的文件类型枚举
     */
    enum FileType {
        IMAGE_PNG,      // PNG图像
        IMAGE_JPG,      // JPEG图像
        IMAGE_JPEG,     // JPEG图像
        IMAGE_BMP,      // BMP图像
        IMAGE_GIF,      // GIF图像
        IMAGE_TIFF,     // TIFF图像
        IMAGE_WEBP,     // WebP图像
        DOCUMENT_PDF,   // PDF文档
        UNKNOWN         // 未知格式
    };

    /**
     * @brief 文件处理结果结构体
     */
    struct ProcessResult {
        QList<QImage> images;       // 处理得到的图像列表
        bool success;               // 是否处理成功
        QString errorMessage;       // 错误信息
        int pageCount;              // 页面数量（对于PDF）
        QStringList pageNames;      // 页面名称列表

        ProcessResult() : success(false), pageCount(0) {}
    };

    explicit FileProcessor(QObject *parent = nullptr);

    /**
     * @brief 检查文件格式是否受支持
     * @param filePath 文件路径
     * @return 是否支持
     */
    static bool isFileSupported(const QString &filePath);

    /**
     * @brief 获取文件类型
     * @param filePath 文件路径
     * @return 文件类型枚举
     */
    static FileType getFileType(const QString &filePath);

    /**
     * @brief 获取支持的文件扩展名列表
     * @return 支持的扩展名列表
     */
    static QStringList getSupportedExtensions();

    /**
     * @brief 获取文件过滤器字符串（用于文件对话框）
     * @return 过滤器字符串
     */
    static QString getFileFilter();

    /**
     * @brief 处理文件，转换为图像
     * @param filePath 输入文件路径
     * @param maxWidth 最大宽度（0表示不限制）
     * @param maxHeight 最大高度（0表示不限制）
     * @return 处理结果
     */
    ProcessResult processFile(const QString &filePath,
                             int maxWidth = 0,
                             int maxHeight = 0);

    /**
     * @brief 处理图像文件
     * @param filePath 图像文件路径
     * @param maxWidth 最大宽度
     * @param maxHeight 最大高度
     * @return 处理结果
     */
    ProcessResult processImageFile(const QString &filePath,
                                  int maxWidth = 0,
                                  int maxHeight = 0);

    /**
     * @brief 处理PDF文件
     * @param filePath PDF文件路径
     * @param maxWidth 最大宽度
     * @param maxHeight 最大高度
     * @return 处理结果
     */
    ProcessResult processPDFFile(const QString &filePath,
                                int maxWidth = 0,
                                int maxHeight = 0);

    /**
     * @brief 调整图像大小（保持宽高比）
     * @param image 原始图像
     * @param maxWidth 最大宽度
     * @param maxHeight 最大高度
     * @return 调整后的图像
     */
    static QImage resizeImage(const QImage &image, int maxWidth, int maxHeight);

signals:
    /**
     * @brief 文件处理进度信号
     * @param progress 进度百分比（0-100）
     * @param currentPage 当前处理页面
     * @param totalPages 总页面数
     */
    void progressUpdated(int progress, int currentPage, int totalPages);

    /**
     * @brief 文件处理完成信号
     * @param result 处理结果
     */
    void processingCompleted(const ProcessResult &result);

    /**
     * @brief 错误信号
     * @param errorMessage 错误信息
     */
    void errorOccurred(const QString &errorMessage);

private:
    /**
     * @brief 使用Poppler转换PDF为图像
     * @param pdfPath PDF文件路径
     * @param outputDir 输出目录
     * @return 转换得到的图像文件路径列表
     */
    QStringList convertPDFToImagesWithPoppler(const QString &pdfPath, const QString &outputDir);

    /**
     * @brief 检查Poppler是否可用
     * @return 是否可用
     */
    bool isPopplerAvailable();

    /**
     * @brief 获取Poppler pdftoppm可执行文件路径
     * @return Poppler pdftoppm可执行文件路径
     */
    QString getPopplerPath() const;

    /**
     * @brief 设置Poppler路径
     * @param path Poppler安装路径
     */
    void setPopplerPath(const QString &path);

    /**
     * @brief 清理临时文件
     * @param filePaths 要清理的文件路径列表
     */
    void cleanupTempFiles(const QStringList &filePaths);

private:
    QStringList m_tempFiles;        // 临时文件列表
    QString m_popplerPath;          // Poppler pdftoppm安装路径
    static QStringList s_imageExtensions;  // 支持的图像扩展名
    static QStringList s_documentExtensions; // 支持的文档扩展名
};

#endif // FILEPROCESSOR_H