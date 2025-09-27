#include "tesseractocrengine.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QApplication>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QFile>

// 常用语言代码映射表
const QMap<QString, QString> TesseractOCREngine::s_languageMap = {
    {"简体中文", "chi_sim"},
    {"简体中文(竖排)", "chi_sim_vert"},
    {"繁体中文", "chi_tra"},
    {"繁体中文(竖排)", "chi_tra_vert"},
    {"英语", "eng"}
};

/**
 * @brief TesseractOCREngine构造函数
 * @param parent 父对象指针
 */
TesseractOCREngine::TesseractOCREngine(QObject *parent)
    : OCREngine(parent)
    , m_tesseractPath("tesseract")  // 默认从PATH中查找
    , m_tessDataPath("")            // 默认使用系统路径
    , m_ocrEngineMode(3)            // 默认OCR引擎模式
    , m_pageSegmentationMode(3)     // 默认页面分割模式
    , m_tesseractProcess(nullptr)
    , m_processingAsync(false)
{
    // 检测bundled版本的Tesseract（支持Enigma Virtual Box）
    QString appDir = QApplication::applicationDirPath();

    // 多种路径尝试（适应不同的虚拟化环境）
    QStringList possiblePaths = {
        appDir + "/tesseract/tesseract.exe",           // 标准路径
        appDir + "/../tesseract/tesseract.exe",        // 上级目录
        appDir + "/../../tesseract/tesseract.exe",     // 上上级目录
        "./tesseract/tesseract.exe",                   // 当前目录相对路径
        "tesseract/tesseract.exe"                      // 简单相对路径
    };

    QString bundledTesseractPath;
    QString bundledTessDataPath;

    // 遍历所有可能的路径
    for (const QString &path : possiblePaths) {
        if (QFileInfo::exists(path)) {
            bundledTesseractPath = path;
            QFileInfo fileInfo(path);
            QString tesseractDir = fileInfo.absolutePath();
            bundledTessDataPath = tesseractDir + "/tessdata";
            break;
        }
    }

    if (!bundledTesseractPath.isEmpty()) {
        m_tesseractPath = bundledTesseractPath;

        // 如果存在bundled tessdata目录，也使用它
        if (QDir(bundledTessDataPath).exists()) {
            m_tessDataPath = bundledTessDataPath;
        }
    }

    m_tesseractProcess = new QProcess(this);

    // 连接信号和槽
    connect(m_tesseractProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TesseractOCREngine::onTesseractFinished);
    connect(m_tesseractProcess, &QProcess::errorOccurred,
            this, &TesseractOCREngine::onTesseractError);
}

/**
 * @brief TesseractOCREngine析构函数
 */
TesseractOCREngine::~TesseractOCREngine()
{
    // 清理临时文件
    cleanupTempFiles();

    // 如果进程仍在运行，终止它
    if (m_tesseractProcess && m_tesseractProcess->state() != QProcess::NotRunning) {
        m_tesseractProcess->terminate();
        if (!m_tesseractProcess->waitForFinished(3000)) {
            m_tesseractProcess->kill();
        }
    }
}

/**
 * @brief 获取引擎类型
 * @return TESSERACT引擎类型
 */
OCREngine::EngineType TesseractOCREngine::getEngineType() const
{
    return TESSERACT;
}

/**
 * @brief 获取引擎名称
 * @return 引擎显示名称
 */
QString TesseractOCREngine::getEngineName() const
{
    return "Tesseract OCR";
}

/**
 * @brief 初始化Tesseract OCR引擎
 * @return 是否初始化成功
 */
bool TesseractOCREngine::initialize()
{
    if (m_initialized) {
        return true;
    }

    // 检查Tesseract是否已安装
    if (!checkTesseractInstallation()) {
        m_lastError = "Tesseract OCR 未安装或无法找到可执行文件";
        emit errorOccurred(m_lastError);
        return false;
    }

    // 获取版本信息
    QString version = getTesseractVersion();
    if (version.isEmpty()) {
        m_lastError = "无法获取Tesseract版本信息";
        emit errorOccurred(m_lastError);
        return false;
    }

    qDebug() << "Tesseract OCR 初始化成功，版本:" << version;
    m_initialized = true;
    return true;
}

/**
 * @brief 执行OCR识别
 * @param image 待识别的图像
 * @param language 识别语言代码
 * @return OCR识别结果
 */
OCREngine::OCRResult TesseractOCREngine::performOCR(const QImage &image, const QString &language)
{
    OCRResult result;

    if (!m_initialized) {
        if (!initialize()) {
            result.success = false;
            result.errorMessage = m_lastError;
            return result;
        }
    }

    if (image.isNull()) {
        result.success = false;
        result.errorMessage = "输入图像为空";
        return result;
    }

    // 保存图像到临时文件
    QString tempImagePath = saveImageToTempFile(image);
    if (tempImagePath.isEmpty()) {
        result.success = false;
        result.errorMessage = "无法保存临时图像文件";
        return result;
    }

    // 准备输出文件路径
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString outputBaseName = tempDir + "/ocr_result_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QString outputPath = outputBaseName + ".txt";
    QString tsvOutputPath = outputBaseName + ".tsv";

    // 准备Tesseract命令参数
    QStringList arguments;
    arguments << tempImagePath;                                    // 输入图像文件
    arguments << outputBaseName;                                   // 输出文件基名（不含扩展名）
    arguments << "-l" << language;                                 // 语言参数
    arguments << "--oem" << QString::number(m_ocrEngineMode);      // OCR引擎模式
    arguments << "--psm" << QString::number(m_pageSegmentationMode); // 页面分割模式
    arguments << "txt" << "tsv";                                   // 同时输出txt和tsv格式

    // 如果指定了tessdata路径，添加到参数中
    if (!m_tessDataPath.isEmpty()) {
        arguments << "--tessdata-dir" << m_tessDataPath;
    }

    // 设置bundled版本的工作目录和环境变量（支持虚拟化环境）
    if (m_tesseractPath.contains("tesseract") && m_tesseractPath.contains("tesseract.exe")) {
        QFileInfo tesseractFile(m_tesseractPath);
        QString tesseractDir = tesseractFile.absolutePath();

        // 确保目录存在
        if (QDir(tesseractDir).exists()) {
            m_tesseractProcess->setWorkingDirectory(tesseractDir);
        }

        // 清除可能干扰的环境变量并添加DLL搜索路径
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove("TESSDATA_PREFIX");

        // 添加tesseract目录到PATH，确保DLL能被找到
        QString currentPath = env.value("PATH");
        env.insert("PATH", tesseractDir + ";" + currentPath);
        m_tesseractProcess->setProcessEnvironment(env);
    }

    // 启动Tesseract进程
    emit progressUpdated(10);
    m_tesseractProcess->start(m_tesseractPath, arguments);

    if (!m_tesseractProcess->waitForStarted(5000)) {
        result.success = false;
        result.errorMessage = "无法启动Tesseract进程: " + m_tesseractProcess->errorString();
        cleanupTempFiles();
        return result;
    }

    emit progressUpdated(20);

    // 等待进程完成，期间定期更新进度
    int elapsedTime = 0;
    const int maxWaitTime = 30000; // 30秒超时
    const int updateInterval = 500; // 每500毫秒更新一次进度

    while (m_tesseractProcess->state() == QProcess::Running && elapsedTime < maxWaitTime) {
        m_tesseractProcess->waitForFinished(updateInterval);
        elapsedTime += updateInterval;

        // 计算进度: 20% -> 75% 根据已用时间
        int currentProgress = 20 + (55 * elapsedTime) / maxWaitTime;
        emit progressUpdated(currentProgress);
    }

    // 检查是否超时
    if (m_tesseractProcess->state() == QProcess::Running) {
        result.success = false;
        result.errorMessage = "Tesseract处理超时";
        m_tesseractProcess->terminate();
        m_tesseractProcess->waitForFinished(3000); // 等待3秒让进程优雅退出
        cleanupTempFiles();
        return result;
    }

    emit progressUpdated(80);

    // 检查进程是否成功执行
    if (m_tesseractProcess->exitStatus() != QProcess::NormalExit ||
        m_tesseractProcess->exitCode() != 0) {
        result.success = false;
        result.errorMessage = "Tesseract执行失败: " +
                             QString::fromUtf8(m_tesseractProcess->readAllStandardError());
        cleanupTempFiles();
        return result;
    }

    // 读取OCR结果
    QString ocrText = readOCRResultFromFile(outputPath);
    if (ocrText.isEmpty()) {
        result.success = false;
        result.errorMessage = "无法读取OCR结果文件";
        cleanupTempFiles();
        return result;
    }

    // 从TSV文件解析置信度
    float confidence = parseConfidenceFromTSV(tsvOutputPath);

    // 清理临时文件
    QFile::remove(tempImagePath);
    QFile::remove(outputPath);
    QFile::remove(tsvOutputPath);

    emit progressUpdated(100);

    // 设置结果
    result.success = true;
    result.text = ocrText.trimmed();
    result.confidence = confidence; // 使用从TSV解析的真实置信度

    emit ocrCompleted(result);
    return result;
}

/**
 * @brief 执行批量OCR识别
 * @param images 待识别的图像列表
 * @param pageNames 页面名称列表
 * @param language 识别语言代码
 * @return 批量OCR识别结果
 */
OCREngine::BatchOCRResult TesseractOCREngine::performBatchOCR(const QList<QImage> &images,
                                                              const QStringList &pageNames,
                                                              const QString &language)
{
    BatchOCRResult batchResult;
    batchResult.totalPages = images.size();

    if (!m_initialized) {
        if (!initialize()) {
            batchResult.success = false;
            batchResult.errorMessage = m_lastError;
            return batchResult;
        }
    }

    if (images.isEmpty()) {
        batchResult.success = false;
        batchResult.errorMessage = "输入图像列表为空";
        return batchResult;
    }

    // 为每个页面准备名称
    QStringList actualPageNames = pageNames;
    while (actualPageNames.size() < images.size()) {
        actualPageNames.append(QString("页面 %1").arg(actualPageNames.size() + 1));
    }

    QStringList allTexts;
    QList<float> allConfidences;

    // 逐页处理OCR
    for (int i = 0; i < images.size(); ++i) {
        const QImage &image = images[i];
        const QString &pageName = actualPageNames[i];

        if (image.isNull()) {
            // 跳过空图像但记录错误
            allTexts.append(QString("错误: 第%1页图像无效").arg(i + 1));
            allConfidences.append(0.0f);

            // 页面完成进度
            int overallProgress = ((i + 1) * 100) / images.size();
            emit batchProgressUpdated(overallProgress, i + 1, images.size(), 100);
            continue;
        }

        // 发送页面开始进度信号
        int baseProgress = (i * 100) / images.size();
        emit batchProgressUpdated(baseProgress, i + 1, images.size(), 0);

        // 使用专门的批量OCR方法来处理单页，包含进度更新
        OCRResult singleResult = performSinglePageOCRWithBatchProgress(image, language, i, images.size());

        if (singleResult.success) {
            allTexts.append(singleResult.text);
            allConfidences.append(singleResult.confidence);
            batchResult.processedPages++;
        } else {
            allTexts.append(QString("错误: %1").arg(singleResult.errorMessage));
            allConfidences.append(0.0f);
        }

        // 页面完成进度
        int overallProgress = ((i + 1) * 100) / images.size();
        emit batchProgressUpdated(overallProgress, i + 1, images.size(), 100);
    }

    // 设置批量结果
    batchResult.texts = allTexts;
    batchResult.pageNames = actualPageNames;
    batchResult.confidences = allConfidences;
    batchResult.success = batchResult.processedPages > 0;

    // 组合所有文本
    QStringList combinedParts;
    for (int i = 0; i < allTexts.size(); ++i) {
        if (!allTexts[i].isEmpty() && !allTexts[i].startsWith("错误:")) {
            combinedParts.append(QString("=== %1 ===\n%2")
                                .arg(actualPageNames[i])
                                .arg(allTexts[i]));
        }
    }
    batchResult.combinedText = combinedParts.join("\n\n");

    if (batchResult.processedPages == 0) {
        batchResult.errorMessage = "所有页面处理失败";
    } else if (batchResult.processedPages < images.size()) {
        batchResult.errorMessage = QString("部分页面处理失败: 成功 %1/%2 页")
                                  .arg(batchResult.processedPages)
                                  .arg(images.size());
    }

    // 发送批量完成信号
    emit batchOcrCompleted(batchResult);
    return batchResult;
}

/**
 * @brief 检查引擎是否可用
 * @return 是否可用
 */
bool TesseractOCREngine::isAvailable() const
{
    return checkTesseractInstallation();
}

/**
 * @brief 获取支持的语言列表
 * @return 支持的语言代码列表
 */
QStringList TesseractOCREngine::getSupportedLanguages() const
{
    QStringList languages;

    // 通过tesseract --list-langs命令获取支持的语言
    QProcess process;
    process.start(m_tesseractPath, QStringList() << "--list-langs");

    if (process.waitForFinished(5000)) {
        QByteArray output = process.readAllStandardOutput();
        QString outputStr = QString::fromUtf8(output);
        QStringList lines = outputStr.split('\n', Qt::SkipEmptyParts);

        // 跳过第一行（通常是"List of available languages (X):"）
        for (int i = 1; i < lines.size(); ++i) {
            QString lang = lines[i].trimmed();
            if (!lang.isEmpty()) {
                languages << lang;
            }
        }
    } else {
        // 如果无法获取，返回默认支持的语言
        languages = s_languageMap.values();
    }

    return languages;
}

/**
 * @brief 设置Tesseract可执行文件路径
 * @param path 可执行文件路径
 */
void TesseractOCREngine::setTesseractPath(const QString &path)
{
    m_tesseractPath = path;
    m_initialized = false; // 重置初始化状态
}

/**
 * @brief 设置tessdata目录路径
 * @param path tessdata目录路径
 */
void TesseractOCREngine::setTessDataPath(const QString &path)
{
    m_tessDataPath = path;
}

/**
 * @brief 设置OCR引擎模式
 * @param mode 引擎模式 (0-13)
 */
void TesseractOCREngine::setOCREngineMode(int mode)
{
    m_ocrEngineMode = qBound(0, mode, 13);
}

/**
 * @brief 设置页面分割模式
 * @param mode 分割模式 (0-13)
 */
void TesseractOCREngine::setPageSegmentationMode(int mode)
{
    m_pageSegmentationMode = qBound(0, mode, 13);
}

/**
 * @brief 处理Tesseract进程完成信号（异步模式用）
 */
void TesseractOCREngine::onTesseractFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    // 异步处理完成的逻辑可以在这里实现
    if (m_processingAsync) {
        m_processingAsync = false;
        // 处理异步结果...
    }
}

/**
 * @brief 处理Tesseract进程错误信号
 */
void TesseractOCREngine::onTesseractError(QProcess::ProcessError error)
{
    QString errorString;
    switch (error) {
        case QProcess::FailedToStart:
            errorString = "无法启动Tesseract进程";
            break;
        case QProcess::Crashed:
            errorString = "Tesseract进程崩溃";
            break;
        case QProcess::Timedout:
            errorString = "Tesseract进程超时";
            break;
        case QProcess::WriteError:
            errorString = "Tesseract进程写入错误";
            break;
        case QProcess::ReadError:
            errorString = "Tesseract进程读取错误";
            break;
        default:
            errorString = "Tesseract进程未知错误";
            break;
    }

    m_lastError = errorString;
    emit errorOccurred(errorString);
}

/**
 * @brief 检查Tesseract是否已安装
 * @return 是否安装
 */
bool TesseractOCREngine::checkTesseractInstallation() const
{
    QProcess testProcess;

    // 如果是bundled版本，设置正确的工作目录和环境
    if (m_tesseractPath.contains("tesseract") && m_tesseractPath.contains("tesseract.exe")) {
        QFileInfo tesseractFile(m_tesseractPath);
        QString tesseractDir = tesseractFile.absolutePath();

        // 设置工作目录（重要：DLL搜索路径）
        testProcess.setWorkingDirectory(tesseractDir);

        // 设置环境变量（清除可能冲突的系统配置）
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove("TESSDATA_PREFIX");

        // 添加当前目录到PATH，确保DLL能被找到
        QString currentPath = env.value("PATH");
        env.insert("PATH", tesseractDir + ";" + currentPath);
        testProcess.setProcessEnvironment(env);
    }

    testProcess.start(m_tesseractPath, QStringList() << "--version");
    return testProcess.waitForFinished(5000) && testProcess.exitCode() == 0;
}

/**
 * @brief 获取Tesseract版本信息
 * @return 版本字符串
 */
QString TesseractOCREngine::getTesseractVersion()
{
    QProcess versionProcess;

    // 如果是bundled版本，设置正确的工作目录和环境
    if (m_tesseractPath.contains("tesseract") && m_tesseractPath.contains("tesseract.exe")) {
        QFileInfo tesseractFile(m_tesseractPath);
        QString tesseractDir = tesseractFile.absolutePath();

        // 设置工作目录（重要：DLL搜索路径）
        versionProcess.setWorkingDirectory(tesseractDir);

        // 设置环境变量（清除可能冲突的系统配置）
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove("TESSDATA_PREFIX");

        // 添加当前目录到PATH，确保DLL能被找到
        QString currentPath = env.value("PATH");
        env.insert("PATH", tesseractDir + ";" + currentPath);
        versionProcess.setProcessEnvironment(env);
    }

    versionProcess.start(m_tesseractPath, QStringList() << "--version");

    if (versionProcess.waitForFinished(5000)) {
        QByteArray output = versionProcess.readAllStandardOutput();
        QString versionStr = QString::fromUtf8(output);

        // 提取版本号（通常在第一行）
        QStringList lines = versionStr.split('\n');
        if (!lines.isEmpty()) {
            return lines.first().trimmed();
        }
    }

    return QString();
}

/**
 * @brief 保存图像到临时文件
 * @param image 要保存的图像
 * @return 临时文件路径
 */
QString TesseractOCREngine::saveImageToTempFile(const QImage &image)
{
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFilePath = tempDir + "/ocr_temp_" +
                          QString::number(QDateTime::currentMSecsSinceEpoch()) + ".png";

    if (image.save(tempFilePath, "PNG")) {
        m_tempFiles << tempFilePath;
        return tempFilePath;
    }

    return QString();
}

/**
 * @brief 从文件读取OCR结果
 * @param filePath 结果文件路径
 * @return 读取到的文本内容
 */
QString TesseractOCREngine::readOCRResultFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    return in.readAll();
}

/**
 * @brief 清理临时文件
 */
void TesseractOCREngine::cleanupTempFiles()
{
    for (const QString &filePath : m_tempFiles) {
        QFile::remove(filePath);
    }
    m_tempFiles.clear();
}

float TesseractOCREngine::parseConfidenceFromTSV(const QString &tsvFilePath)
{
    QFile file(tsvFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0.8f; // 如果无法读取TSV文件，返回默认值
    }

    QTextStream in(&file);
    QStringList lines = in.readAll().split('\n', Qt::SkipEmptyParts);

    if (lines.size() <= 1) {
        return 0.8f; // 没有数据行，返回默认值
    }

    // TSV格式：level page_num block_num par_num line_num word_num left top width height conf text
    // 置信度在第11列（索引10）
    float totalConfidence = 0.0f;
    int wordCount = 0;

    // 跳过第一行（标题行）
    for (int i = 1; i < lines.size(); ++i) {
        QStringList columns = lines[i].split('\t');

        // 确保有足够的列且不是空行
        if (columns.size() >= 11) {
            bool ok;
            float conf = columns[10].toFloat(&ok);

            // 只处理有效的单词级别数据（level=5）且置信度>=0
            if (ok && conf >= 0 && columns.size() > 11 && !columns[11].trimmed().isEmpty()) {
                totalConfidence += conf;
                wordCount++;
            }
        }
    }

    file.close();

    if (wordCount > 0) {
        // 返回平均置信度，转换为0-1范围
        return totalConfidence / (wordCount * 100.0f);
    }

    return 0.8f; // 没有有效数据，返回默认值
}

/**
 * @brief 执行单页OCR识别（专用于批量处理，包含批量进度更新）
 * @param image 待识别的图像
 * @param language 识别语言代码
 * @param pageIndex 当前页面索引
 * @param totalPages 总页面数
 * @return OCR识别结果
 */
OCREngine::OCRResult TesseractOCREngine::performSinglePageOCRWithBatchProgress(const QImage &image,
                                                                               const QString &language,
                                                                               int pageIndex,
                                                                               int totalPages)
{
    OCRResult result;

    if (!m_initialized) {
        if (!initialize()) {
            result.success = false;
            result.errorMessage = m_lastError;
            return result;
        }
    }

    if (image.isNull()) {
        result.success = false;
        result.errorMessage = "输入图像为空";
        return result;
    }

    // 保存图像到临时文件
    QString tempImagePath = saveImageToTempFile(image);
    if (tempImagePath.isEmpty()) {
        result.success = false;
        result.errorMessage = "无法保存临时图像文件";
        return result;
    }

    // 准备输出文件路径
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString outputBaseName = tempDir + "/ocr_result_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QString outputPath = outputBaseName + ".txt";
    QString tsvOutputPath = outputBaseName + ".tsv";

    // 准备Tesseract命令参数
    QStringList arguments;
    arguments << tempImagePath;                                    // 输入图像文件
    arguments << outputBaseName;                                   // 输出文件基名（不含扩展名）
    arguments << "-l" << language;                                 // 语言参数
    arguments << "--oem" << QString::number(m_ocrEngineMode);      // OCR引擎模式
    arguments << "--psm" << QString::number(m_pageSegmentationMode); // 页面分割模式
    arguments << "txt" << "tsv";                                   // 同时输出txt和tsv格式

    // 如果指定了tessdata路径，添加到参数中
    if (!m_tessDataPath.isEmpty()) {
        arguments << "--tessdata-dir" << m_tessDataPath;
    }

    // 设置bundled版本的工作目录和环境变量（用于DLL搜索）
    if (m_tesseractPath.contains("tesseract") && m_tesseractPath.contains("tesseract.exe")) {
        QString tesseractDir = QFileInfo(m_tesseractPath).absolutePath();
        m_tesseractProcess->setWorkingDirectory(tesseractDir);

        // 设置环境变量，确保DLL能被找到
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.remove("TESSDATA_PREFIX");
        QString currentPath = env.value("PATH");
        env.insert("PATH", tesseractDir + ";" + currentPath);
        m_tesseractProcess->setProcessEnvironment(env);

        qDebug() << "设置Tesseract工作目录:" << tesseractDir;
        qDebug() << "使用Tesseract路径:" << m_tesseractPath;
    }

    // 启动Tesseract进程并发送批量进度信号
    int currentProgress = (pageIndex * 100 + 10) / totalPages; // 10% 为启动进度
    emit batchProgressUpdated(currentProgress, pageIndex + 1, totalPages, 10);

    m_tesseractProcess->start(m_tesseractPath, arguments);

    if (!m_tesseractProcess->waitForStarted(5000)) {
        result.success = false;
        result.errorMessage = "无法启动Tesseract进程: " + m_tesseractProcess->errorString();
        cleanupTempFiles();
        return result;
    }

    currentProgress = (pageIndex * 100 + 20) / totalPages; // 20% 为已启动
    emit batchProgressUpdated(currentProgress, pageIndex + 1, totalPages, 20);

    // 等待进程完成，期间定期更新批量进度
    int elapsedTime = 0;
    const int maxWaitTime = 30000; // 30秒超时
    const int updateInterval = 500; // 每500毫秒更新一次进度

    while (m_tesseractProcess->state() == QProcess::Running && elapsedTime < maxWaitTime) {
        m_tesseractProcess->waitForFinished(updateInterval);
        elapsedTime += updateInterval;

        // 计算当前页面进度: 20% -> 75%
        int pageProgress = 20 + (55 * elapsedTime) / maxWaitTime;
        currentProgress = (pageIndex * 100 + pageProgress) / totalPages;
        emit batchProgressUpdated(currentProgress, pageIndex + 1, totalPages, pageProgress);
    }

    // 检查是否超时
    if (m_tesseractProcess->state() == QProcess::Running) {
        result.success = false;
        result.errorMessage = "Tesseract处理超时";
        m_tesseractProcess->terminate();
        m_tesseractProcess->waitForFinished(3000); // 等待3秒让进程优雅退出
        cleanupTempFiles();
        return result;
    }

    currentProgress = (pageIndex * 100 + 80) / totalPages; // 80% 为处理完成
    emit batchProgressUpdated(currentProgress, pageIndex + 1, totalPages, 80);

    // 检查进程是否成功执行
    if (m_tesseractProcess->exitStatus() != QProcess::NormalExit ||
        m_tesseractProcess->exitCode() != 0) {
        result.success = false;
        result.errorMessage = "Tesseract执行失败: " +
                             QString::fromUtf8(m_tesseractProcess->readAllStandardError());
        cleanupTempFiles();
        return result;
    }

    // 读取OCR结果
    QString ocrText = readOCRResultFromFile(outputPath);
    if (ocrText.isEmpty()) {
        result.success = false;
        result.errorMessage = "无法读取OCR结果文件";
        cleanupTempFiles();
        return result;
    }

    // 从TSV文件解析置信度
    float confidence = parseConfidenceFromTSV(tsvOutputPath);

    // 清理临时文件
    QFile::remove(tempImagePath);
    QFile::remove(outputPath);
    QFile::remove(tsvOutputPath);

    currentProgress = (pageIndex * 100 + 100) / totalPages; // 100% 为完全完成
    emit batchProgressUpdated(currentProgress, pageIndex + 1, totalPages, 100);

    // 设置结果
    result.success = true;
    result.text = ocrText.trimmed();
    result.confidence = confidence;

    return result;
}