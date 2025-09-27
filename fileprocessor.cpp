#include "fileprocessor.h"
#include <QImageReader>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QDebug>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QApplication>
#include <QFileInfo>
#include <algorithm>

// 静态成员变量初始化
QStringList FileProcessor::s_imageExtensions = {
    "png", "jpg", "jpeg", "bmp", "gif", "tiff", "tif", "webp"
};

QStringList FileProcessor::s_documentExtensions = {
    "pdf"
};

/**
 * @brief FileProcessor构造函数
 * @param parent 父对象指针
 */
FileProcessor::FileProcessor(QObject *parent)
    : QObject(parent)
    , m_popplerPath("D:/poppler-25.07.0/bin/pdftoppm.exe")
{
    // 检测bundled版本的Poppler
    QString appDir = QApplication::applicationDirPath();
    QString bundledPopplerPath = appDir + "/poppler/pdftoppm.exe";

    if (QFileInfo::exists(bundledPopplerPath)) {
        m_popplerPath = bundledPopplerPath;
        qDebug() << "使用bundled版本Poppler:" << m_popplerPath;
    } else if (!QFile::exists(m_popplerPath)) {
        // 如果默认路径和bundled路径都不存在，尝试从PATH环境变量中查找
        m_popplerPath = "pdftoppm"; // 回退到PATH中的pdftoppm命令
        qDebug() << "未发现bundled版本Poppler，使用系统安装版本";
    } else {
        qDebug() << "使用默认路径Poppler:" << m_popplerPath;
    }
}

/**
 * @brief 检查文件格式是否受支持
 * @param filePath 文件路径
 * @return 是否支持该文件格式
 */
bool FileProcessor::isFileSupported(const QString &filePath)
{
    return getFileType(filePath) != UNKNOWN;
}

/**
 * @brief 获取文件类型
 * @param filePath 文件路径
 * @return 文件类型枚举值
 */
FileProcessor::FileType FileProcessor::getFileType(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    // 检查是否为图像格式
    if (extension == "png") return IMAGE_PNG;
    if (extension == "jpg") return IMAGE_JPG;
    if (extension == "jpeg") return IMAGE_JPEG;
    if (extension == "bmp") return IMAGE_BMP;
    if (extension == "gif") return IMAGE_GIF;
    if (extension == "tiff" || extension == "tif") return IMAGE_TIFF;
    if (extension == "webp") return IMAGE_WEBP;

    // 检查是否为文档格式
    if (extension == "pdf") return DOCUMENT_PDF;

    return UNKNOWN;
}

/**
 * @brief 获取支持的文件扩展名列表
 * @return 扩展名列表
 */
QStringList FileProcessor::getSupportedExtensions()
{
    QStringList extensions = s_imageExtensions;
    extensions.append(s_documentExtensions);
    return extensions;
}

/**
 * @brief 获取文件过滤器字符串（用于文件选择对话框）
 * @return 过滤器字符串
 */
QString FileProcessor::getFileFilter()
{
    QString imageFilter = "图像文件 (";
    for (const QString &ext : s_imageExtensions) {
        imageFilter += "*." + ext + " ";
    }
    imageFilter.chop(1); // 移除最后的空格
    imageFilter += ")";

    QString documentFilter = "文档文件 (";
    for (const QString &ext : s_documentExtensions) {
        documentFilter += "*." + ext + " ";
    }
    documentFilter.chop(1);
    documentFilter += ")";

    QString allSupportedFilter = "所有支持的文件 (";
    QStringList allExtensions = getSupportedExtensions();
    for (const QString &ext : allExtensions) {
        allSupportedFilter += "*." + ext + " ";
    }
    allSupportedFilter.chop(1);
    allSupportedFilter += ")";

    return allSupportedFilter + ";;" + imageFilter + ";;" + documentFilter + ";;所有文件 (*.*)";
}

/**
 * @brief 处理文件，将其转换为图像列表
 * @param filePath 输入文件路径
 * @param maxWidth 最大宽度限制
 * @param maxHeight 最大高度限制
 * @return 处理结果
 */
FileProcessor::ProcessResult FileProcessor::processFile(const QString &filePath,
                                                       int maxWidth,
                                                       int maxHeight)
{
    ProcessResult result;

    if (!QFile::exists(filePath)) {
        result.success = false;
        result.errorMessage = "文件不存在: " + filePath;
        emit errorOccurred(result.errorMessage);
        return result;
    }

    FileType fileType = getFileType(filePath);
    emit progressUpdated(10, 0, 1);

    switch (fileType) {
        case IMAGE_PNG:
        case IMAGE_JPG:
        case IMAGE_JPEG:
        case IMAGE_BMP:
        case IMAGE_GIF:
        case IMAGE_TIFF:
        case IMAGE_WEBP:
            result = processImageFile(filePath, maxWidth, maxHeight);
            break;

        case DOCUMENT_PDF:
            result = processPDFFile(filePath, maxWidth, maxHeight);
            break;

        default:
            result.success = false;
            result.errorMessage = "不支持的文件格式: " + QFileInfo(filePath).suffix();
            emit errorOccurred(result.errorMessage);
            break;
    }

    if (result.success) {
        emit processingCompleted(result);
    }

    return result;
}

/**
 * @brief 处理图像文件
 * @param filePath 图像文件路径
 * @param maxWidth 最大宽度
 * @param maxHeight 最大高度
 * @return 处理结果
 */
FileProcessor::ProcessResult FileProcessor::processImageFile(const QString &filePath,
                                                            int maxWidth,
                                                            int maxHeight)
{
    ProcessResult result;

    QImageReader reader(filePath);
    if (!reader.canRead()) {
        result.success = false;
        result.errorMessage = "无法读取图像文件: " + reader.errorString();
        return result;
    }

    emit progressUpdated(30, 1, 1);

    QImage image = reader.read();
    if (image.isNull()) {
        result.success = false;
        result.errorMessage = "图像文件损坏或格式不正确";
        return result;
    }

    emit progressUpdated(60, 1, 1);

    // 如果指定了大小限制，调整图像大小
    if ((maxWidth > 0 || maxHeight > 0) && (image.width() > maxWidth || image.height() > maxHeight)) {
        image = resizeImage(image, maxWidth, maxHeight);
    }

    emit progressUpdated(90, 1, 1);

    // 设置结果
    result.success = true;
    result.images.append(image);
    result.pageCount = 1;
    result.pageNames.append(QFileInfo(filePath).baseName());

    emit progressUpdated(100, 1, 1);

    return result;
}

/**
 * @brief 处理PDF文件
 * @param filePath PDF文件路径
 * @param maxWidth 最大宽度
 * @param maxHeight 最大高度
 * @return 处理结果
 */
FileProcessor::ProcessResult FileProcessor::processPDFFile(const QString &filePath,
                                                          int maxWidth,
                                                          int maxHeight)
{
    ProcessResult result;

    // 检查Poppler是否可用
    if (!isPopplerAvailable()) {
        result.success = false;
        result.errorMessage = "Poppler不可用。请确保Poppler已正确安装并配置路径";
        return result;
    }

    emit progressUpdated(5, 0, 0);

    // 创建临时目录
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString outputDir = tempDir + "/ocr_pdf_" + QString::number(QDateTime::currentMSecsSinceEpoch());

    QDir().mkpath(outputDir);
    emit progressUpdated(10, 0, 0);

    // 使用Poppler转换PDF为图像
    QStringList imageFiles = convertPDFToImagesWithPoppler(filePath, outputDir);
    qDebug() << "PDF转换结果: 共生成" << imageFiles.size() << "个图像文件";
    qDebug() << "图像文件列表:" << imageFiles;

    if (imageFiles.isEmpty()) {
        result.success = false;
        result.errorMessage = "无法转换PDF文件";
        QDir(outputDir).removeRecursively();
        return result;
    }

    emit progressUpdated(40, 0, imageFiles.size());

    // 加载转换得到的图像
    for (int i = 0; i < imageFiles.size(); ++i) {
        const QString &imageFile = imageFiles[i];
        QImage image(imageFile);

        if (!image.isNull()) {
            // 调整图像大小（如果需要）
            if ((maxWidth > 0 || maxHeight > 0) &&
                (image.width() > maxWidth || image.height() > maxHeight)) {
                image = resizeImage(image, maxWidth, maxHeight);
            }

            result.images.append(image);
            result.pageNames.append(QString("页面 %1").arg(i + 1));
        }

        emit progressUpdated(50 + (50 * (i + 1)) / imageFiles.size(), i + 1, imageFiles.size());
    }

    // 清理临时文件
    cleanupTempFiles(imageFiles);
    QDir(outputDir).removeRecursively();

    // 设置结果
    result.success = !result.images.isEmpty();
    result.pageCount = result.images.size();

    if (!result.success) {
        result.errorMessage = "无法从PDF文件中提取图像";
    }

    return result;
}

/**
 * @brief 调整图像大小，保持宽高比
 * @param image 原始图像
 * @param maxWidth 最大宽度
 * @param maxHeight 最大高度
 * @return 调整后的图像
 */
QImage FileProcessor::resizeImage(const QImage &image, int maxWidth, int maxHeight)
{
    if (image.isNull()) {
        return image;
    }

    // 如果没有指定限制，返回原图
    if (maxWidth <= 0 && maxHeight <= 0) {
        return image;
    }

    QSize targetSize = image.size();

    // 如果指定了最大宽度
    if (maxWidth > 0 && targetSize.width() > maxWidth) {
        targetSize.scale(maxWidth, targetSize.height(), Qt::KeepAspectRatio);
    }

    // 如果指定了最大高度
    if (maxHeight > 0 && targetSize.height() > maxHeight) {
        targetSize.scale(targetSize.width(), maxHeight, Qt::KeepAspectRatio);
    }

    // 如果大小没有改变，返回原图
    if (targetSize == image.size()) {
        return image;
    }

    return image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

/**
 * @brief 使用Poppler将PDF转换为图像
 * @param pdfPath PDF文件路径
 * @param outputDir 输出目录
 * @return 转换得到的图像文件路径列表
 */
QStringList FileProcessor::convertPDFToImagesWithPoppler(const QString &pdfPath, const QString &outputDir)
{
    QStringList imageFiles;
    QProcess popplerProcess;
    QString outputPrefix = outputDir + "/page";

    // 构建Poppler pdftoppm命令（高质量PNG输出）
    QStringList arguments;
    arguments << "-png";                              // PNG格式输出
    arguments << "-r" << "200";                       // 分辨率200 DPI（高质量）
    arguments << "-aa" << "yes";                      // 开启抗锯齿
    arguments << "-aaVector" << "yes";                // 矢量图形抗锯齿
    arguments << pdfPath;                             // 输入PDF文件
    arguments << outputPrefix;                        // 输出文件前缀

    qDebug() << "Poppler命令:" << m_popplerPath << arguments.join(" ");
    emit progressUpdated(15, 0, 0);

    popplerProcess.start(m_popplerPath, arguments);

    // 监控转换进程
    if (popplerProcess.waitForStarted(10000)) {
        emit progressUpdated(20, 0, 0);
        qDebug() << "Poppler进程已启动";

        // 等待进程完成，期间更新进度
        int elapsedTime = 0;
        const int maxWaitTime = 60000; // 60秒超时时间
        while (popplerProcess.state() == QProcess::Running && elapsedTime < maxWaitTime) {
            popplerProcess.waitForFinished(2000);  // 等待2秒
            elapsedTime += 2000;

            // 根据时间估算进度 (20% -> 35%)
            int estimatedProgress = 20 + (15 * elapsedTime) / maxWaitTime;
            emit progressUpdated(estimatedProgress, 0, 0);
        }
    } else {
        qDebug() << "无法启动Poppler进程:" << popplerProcess.errorString();
        return imageFiles;
    }

    // 如果进程仍在运行，终止它
    if (popplerProcess.state() == QProcess::Running) {
        qDebug() << "Poppler进程超时，正在终止...";
        popplerProcess.terminate();
        if (!popplerProcess.waitForFinished(5000)) {
            popplerProcess.kill();
            popplerProcess.waitForFinished(3000);
        }
    }

    // 检查进程执行结果
    int exitCode = popplerProcess.exitCode();
    QProcess::ExitStatus exitStatus = popplerProcess.exitStatus();

    qDebug() << "Poppler进程退出码:" << exitCode;
    qDebug() << "Poppler进程退出状态:" << exitStatus;

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QString errorOutput = QString::fromLocal8Bit(popplerProcess.readAllStandardError());
        qDebug() << "Poppler错误输出:" << errorOutput;
        return imageFiles;
    }

    emit progressUpdated(35, 0, 0);

    // 查找生成的图像文件
    QDir dir(outputDir);
    QStringList filters;
    // Poppler pdftoppm 生成格式: page-1.png, page-2.png 等
    filters << "page-*.png";
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

    // 如果没找到预期格式，尝试其他可能的格式
    if (files.isEmpty()) {
        filters.clear();
        filters << "*.png";
        files = dir.entryList(filters, QDir::Files, QDir::Name);
    }

    qDebug() << "Poppler生成的文件:" << files;

    // 按页面编号排序确保页面顺序正确
    auto pageNumberComparator = [](const QString &a, const QString &b) {
        // 提取文件名中的页面编号进行比较
        QRegularExpression pageRegex("page-(\\d+)\\.png");
        int pageA = 0, pageB = 0;
        QRegularExpressionMatch matchA = pageRegex.match(a);
        if (matchA.hasMatch()) {
            pageA = matchA.captured(1).toInt();
        }
        QRegularExpressionMatch matchB = pageRegex.match(b);
        if (matchB.hasMatch()) {
            pageB = matchB.captured(1).toInt();
        }
        return pageA < pageB;
    };

    std::sort(files.begin(), files.end(), pageNumberComparator);

    for (const QString &file : files) {
        QString fullPath = dir.absoluteFilePath(file);

        // 验证文件确实存在且不为空
        QFileInfo fileInfo(fullPath);
        if (fileInfo.exists() && fileInfo.size() > 0) {
            imageFiles << fullPath;
            m_tempFiles << fullPath;
            qDebug() << "添加转换结果文件:" << fullPath << "大小:" << fileInfo.size() << "字节";
        } else {
            qDebug() << "跳过无效文件:" << fullPath;
        }
    }

    qDebug() << "PDF转换完成，共生成" << imageFiles.size() << "个图像文件";
    return imageFiles;
}

/**
 * @brief 检查Poppler是否可用
 * @return 是否可用
 */
bool FileProcessor::isPopplerAvailable()
{
    QProcess testProcess;
    testProcess.start(m_popplerPath, QStringList() << "-h");

    bool result = testProcess.waitForFinished(10000) &&
                  testProcess.exitCode() == 0;

    if (result) {
        QString output = QString::fromLocal8Bit(testProcess.readAllStandardOutput());
        qDebug() << "Poppler版本信息:" << output.split('\n').first();
    } else {
        QString errorOutput = QString::fromLocal8Bit(testProcess.readAllStandardError());
        qDebug() << "Poppler不可用:" << testProcess.errorString();
        qDebug() << "错误输出:" << errorOutput;
    }

    return result;
}

/**
 * @brief 清理临时文件
 * @param filePaths 要清理的文件路径列表
 */
void FileProcessor::cleanupTempFiles(const QStringList &filePaths)
{
    for (const QString &filePath : filePaths) {
        QFile::remove(filePath);
    }

    // 同时清理成员变量中的临时文件
    for (const QString &filePath : m_tempFiles) {
        QFile::remove(filePath);
    }
    m_tempFiles.clear();
}

/**
 * @brief 获取Poppler pdftoppm可执行文件路径
 * @return Poppler pdftoppm可执行文件路径
 */
QString FileProcessor::getPopplerPath() const
{
    return m_popplerPath;
}

/**
 * @brief 设置Poppler路径
 * @param path Poppler安装路径
 */
void FileProcessor::setPopplerPath(const QString &path)
{
    // 如果提供的是目录路径，自动添加pdftoppm.exe
    QFileInfo pathInfo(path);
    if (pathInfo.isDir()) {
        m_popplerPath = path + "/bin/pdftoppm.exe";
    } else {
        m_popplerPath = path;
    }

    qDebug() << "Poppler路径设置为:" << m_popplerPath;
}