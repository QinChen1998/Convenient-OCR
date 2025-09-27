#include "mainwindow.h"
#include "ui_mainwindow.h"

// 静态成员变量定义
const QMap<QString, QString> MainWindow::s_languageCodeMap = {
    {"中英混合", "chi_sim+eng"},
    {"简体中文", "chi_sim"},
    {"简体中文(竖排)", "chi_sim_vert"},
    {"繁体中文", "chi_tra"},
    {"繁体中文(竖排)", "chi_tra_vert"},
    {"英语", "eng"}
};

const QMap<QString, QString> MainWindow::s_languageNameMap = {
    {"chi_sim+eng", "中英混合"},
    {"chi_sim", "简体中文"},
    {"chi_sim_vert", "简体中文(竖排)"},
    {"chi_tra", "繁体中文"},
    {"chi_tra_vert", "繁体中文(竖排)"},
    {"eng", "英语"}
};

/**
 * @brief MainWindow构造函数
 * 初始化主窗口及其所有组件
 * @param parent 父窗口指针
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_fileProcessor(nullptr)
    , m_ocrEngine(nullptr)
    , m_tesseractEngine(nullptr)
    , m_currentPageIndex(0)
    , m_isProcessing(false)
    , m_hasValidFile(false)
    , m_windowHiddenForCapture(false)
    , m_statusLabel(nullptr)
    , m_statusProgressBar(nullptr)
{
    ui->setupUi(this);

    // 初始化各个组件
    initUI();
    loadLanguagePreference();  // 加载用户的语言偏好设置
    initOCREngine();
    connectSignalsAndSlots();

    // 更新初始UI状态
    updateUIState(false);
}

/**
 * @brief MainWindow析构函数
 * 清理资源
 */
MainWindow::~MainWindow()
{
    // 清理OCR引擎
    if (m_tesseractEngine) {
        delete m_tesseractEngine;
    }

    // 清理文件处理器
    if (m_fileProcessor) {
        delete m_fileProcessor;
    }

    delete ui;
}

/**
 * @brief 初始化UI组件
 */
void MainWindow::initUI()
{
    // 设置窗口图标（如果图标文件存在）
    setWindowIcon(QIcon(":/icons/app_icon.png"));

    // 初始化状态栏
    m_statusLabel = new QLabel("准备就绪", this);
    m_statusProgressBar = new QProgressBar(this);
    m_statusProgressBar->setVisible(false);
    m_statusProgressBar->setMaximumWidth(200);

    ui->statusbar->addWidget(m_statusLabel, 1);
    ui->statusbar->addWidget(m_statusProgressBar);

    // 设置默认标签页
    ui->tabWidget->setCurrentIndex(0);

    // 隐藏页面导航控件（初始状态）
    ui->btnPrevPage->setVisible(false);
    ui->btnNextPage->setVisible(false);
    ui->lblPageInfo->setVisible(false);

    // 设置图像预览区域的最小大小
    ui->imageScrollArea->setMinimumSize(400, 300);

    // 设置进度条初始状态
    ui->progressBar->setValue(0);

    // 禁用开始识别按钮（初始状态）
    ui->btnStartOCR->setEnabled(false);
}

/**
 * @brief 连接信号和槽
 */
void MainWindow::connectSignalsAndSlots()
{
    // UI控件信号连接
    connect(ui->btnSelectFile, &QPushButton::clicked, this, &MainWindow::onSelectFileClicked);
    connect(ui->btnScreenCapture, &QPushButton::clicked, this, &MainWindow::onScreenCaptureClicked);
    connect(ui->btnStartOCR, &QPushButton::clicked, this, &MainWindow::onStartOCRClicked);
    connect(ui->btnPrevPage, &QPushButton::clicked, this, &MainWindow::onPrevPageClicked);
    connect(ui->btnNextPage, &QPushButton::clicked, this, &MainWindow::onNextPageClicked);
    connect(ui->btnCopyResult, &QPushButton::clicked, this, &MainWindow::onCopyResultClicked);
    connect(ui->btnSaveResult, &QPushButton::clicked, this, &MainWindow::onSaveResultClicked);
    connect(ui->btnClearResult, &QPushButton::clicked, this, &MainWindow::onClearResultClicked);

    // 组合框信号连接
    connect(ui->comboLanguage, &QComboBox::currentTextChanged, this, &MainWindow::onLanguageChanged);
    connect(ui->comboEngine, &QComboBox::currentTextChanged, this, &MainWindow::onEngineChanged);

    // 菜单动作信号连接
    connect(ui->actionOpenFile, &QAction::triggered, this, &MainWindow::onActionOpenFile);
    connect(ui->actionSaveResult, &QAction::triggered, this, &MainWindow::onActionSaveResult);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onActionExit);
    connect(ui->actionCopyResult, &QAction::triggered, this, &MainWindow::onActionCopyResult);
    connect(ui->actionClearResult, &QAction::triggered, this, &MainWindow::onActionClearResult);
    connect(ui->actionOCRSettings, &QAction::triggered, this, &MainWindow::onActionOCRSettings);
    connect(ui->actionLanguageSettings, &QAction::triggered, this, &MainWindow::onActionLanguageSettings);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onActionAbout);
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::onActionHelp);
    connect(ui->actionLicense, &QAction::triggered, this, &MainWindow::onActionLicense);
}

/**
 * @brief 初始化OCR引擎
 */
void MainWindow::initOCREngine()
{
    // 创建文件处理器
    m_fileProcessor = new FileProcessor(this);

    // 连接文件处理器信号
    connect(m_fileProcessor, &FileProcessor::progressUpdated,
            this, &MainWindow::onFileProcessProgress);
    connect(m_fileProcessor, &FileProcessor::processingCompleted,
            this, &MainWindow::onFileProcessCompleted);
    connect(m_fileProcessor, &FileProcessor::errorOccurred,
            this, &MainWindow::onFileProcessError);

    // 创建Tesseract OCR引擎
    m_tesseractEngine = new TesseractOCREngine(this);

    // 连接OCR引擎信号
    connect(m_tesseractEngine, &TesseractOCREngine::progressUpdated,
            this, &MainWindow::onOCRProgress);
    connect(m_tesseractEngine, &TesseractOCREngine::batchProgressUpdated,
            this, &MainWindow::onBatchOCRProgress);
    connect(m_tesseractEngine, &TesseractOCREngine::ocrCompleted,
            this, &MainWindow::onOCRCompleted);
    connect(m_tesseractEngine, &TesseractOCREngine::batchOcrCompleted,
            this, &MainWindow::onBatchOCRCompleted);
    connect(m_tesseractEngine, &TesseractOCREngine::errorOccurred,
            this, &MainWindow::onOCRError);

    // 设置当前使用的OCR引擎
    m_ocrEngine = m_tesseractEngine;

    // 初始化OCR引擎
    if (!m_ocrEngine->initialize()) {
        QMessageBox::warning(this, "警告",
                           "OCR引擎初始化失败。请确保已正确安装Tesseract OCR。\n\n"
                           "您可以从 https://github.com/tesseract-ocr/tesseract 下载安装。");
    }
}

/**
 * @brief 更新UI状态
 * @param hasFile 是否已选择文件
 */
void MainWindow::updateUIState(bool hasFile)
{
    m_hasValidFile = hasFile;

    // 更新按钮状态
    ui->btnStartOCR->setEnabled(hasFile && !m_isProcessing);

    // 更新菜单项状态
    ui->actionSaveResult->setEnabled(!m_currentOCRResult.isEmpty());

    // 更新结果区域按钮状态
    bool hasResult = !m_currentOCRResult.isEmpty();
    ui->btnCopyResult->setEnabled(hasResult);
    ui->btnSaveResult->setEnabled(hasResult);
    ui->btnClearResult->setEnabled(hasResult);

    // 更新页面导航状态
    bool hasMultiplePages = m_loadedImages.size() > 1;
    ui->btnPrevPage->setVisible(hasMultiplePages);
    ui->btnNextPage->setVisible(hasMultiplePages);
    ui->lblPageInfo->setVisible(hasMultiplePages);

    if (hasMultiplePages) {
        ui->btnPrevPage->setEnabled(m_currentPageIndex > 0);
        ui->btnNextPage->setEnabled(m_currentPageIndex < m_loadedImages.size() - 1);
        ui->lblPageInfo->setText(QString("第 %1 页 / 共 %2 页")
                               .arg(m_currentPageIndex + 1)
                               .arg(m_loadedImages.size()));
    }
}

// UI事件处理槽函数实现

/**
 * @brief 选择文件按钮点击处理
 */
void MainWindow::onSelectFileClicked()
{
    QString filter = FileProcessor::getFileFilter();
    QString fileName = QFileDialog::getOpenFileName(this, "选择要识别的文件", "", filter);

    if (!fileName.isEmpty()) {
        // 显示处理进度
        ui->progressBar->setValue(0);
        ui->lblProgressText->setText("正在加载文件...");

        m_isProcessing = true;
        updateUIState(false);

        // 开始处理文件
        m_currentFilePath = fileName;
        QFileInfo fileInfo(fileName);
        ui->lblSelectedFile->setText(fileInfo.fileName());

        // 异步处理文件
        QTimer::singleShot(100, [this]() {
            FileProcessor::ProcessResult result = m_fileProcessor->processFile(m_currentFilePath);
            onFileProcessCompleted(result);
        });
    }
}

/**
 * @brief 屏幕截图按钮点击处理
 */
void MainWindow::onScreenCaptureClicked()
{
    if (m_isProcessing) {
        return;
    }

    // 检查是否需要隐藏窗口
    bool shouldHideWindow = ui->chkHideWindow->isChecked();
    if (shouldHideWindow) {
        m_windowHiddenForCapture = true;
        hide();

        // 给窗口一点时间完全隐藏，然后再开始截图
        QTimer::singleShot(200, [this]() {
            this->startScreenCapture();
        });
    } else {
        startScreenCapture();
    }
}

/**
 * @brief 截图完成处理
 * @param imagePath 截图文件路径，空字符串表示取消
 */
void MainWindow::onScreenCaptureFinished(const QString &imagePath)
{
    // 如果窗口被隐藏了，恢复显示
    if (m_windowHiddenForCapture) {
        show();
        activateWindow();  // 激活窗口，确保它获得焦点
        raise();           // 将窗口置于最前
        m_windowHiddenForCapture = false;
    }

    if (imagePath.isEmpty()) {
        // 用户取消了截图
        return;
    }

    // 将截图文件作为选择的文件进行处理
    m_currentFilePath = imagePath;

    // 显示处理进度
    ui->progressBar->setValue(0);
    ui->lblProgressText->setText("正在加载截图...");

    // 更新文件选择标签
    ui->lblSelectedFile->setText("屏幕截图");

    m_isProcessing = true;
    updateUIState(false);

    // 异步处理截图文件
    QTimer::singleShot(100, [this]() {
        FileProcessor::ProcessResult result = m_fileProcessor->processFile(m_currentFilePath);
        onFileProcessCompleted(result);
    });
}

/**
 * @brief 开始OCR识别按钮点击处理
 */
void MainWindow::onStartOCRClicked()
{
    if (m_loadedImages.isEmpty() || m_isProcessing) {
        return;
    }

    // 获取选择的语言
    QString languageCode = getCurrentLanguageCode();

    // 显示处理进度
    ui->progressBar->setValue(0);
    m_statusProgressBar->setVisible(true);
    m_statusProgressBar->setValue(0);

    m_isProcessing = true;
    updateUIState(m_hasValidFile);

    // 切换到结果标签页
    ui->tabWidget->setCurrentIndex(1);

    // 检查是否为多页文档
    if (m_loadedImages.size() > 1) {
        // 多页文档：使用批量处理
        ui->lblProgressText->setText("正在批量识别所有页面...");
        showStatusMessage("开始批量OCR识别...", 0);

        // 使用异步方式启动批量OCR
        QTimer::singleShot(100, [this, languageCode]() {
            OCREngine::BatchOCRResult result = m_ocrEngine->performBatchOCR(m_loadedImages,
                                                                           m_imageNames,
                                                                           languageCode);
            onBatchOCRCompleted(result);
        });
    } else {
        // 单页文档：使用原有的单页处理
        QImage currentImage = m_loadedImages[m_currentPageIndex];
        if (currentImage.isNull()) {
            QMessageBox::warning(this, "错误", "当前图像无效");
            m_isProcessing = false;
            updateUIState(m_hasValidFile);
            return;
        }

        ui->lblProgressText->setText("正在识别文字...");
        showStatusMessage("开始OCR识别...", 0);

        // 使用异步方式启动单页OCR
        QTimer::singleShot(100, [this, currentImage, languageCode]() {
            OCREngine::OCRResult result = m_ocrEngine->performOCR(currentImage, languageCode);
            if (result.success) {
                onOCRCompleted(result);
            } else {
                onOCRError(result.errorMessage);
            }
        });
    }
}

/**
 * @brief 上一页按钮点击处理
 */
void MainWindow::onPrevPageClicked()
{
    if (m_currentPageIndex > 0) {
        m_currentPageIndex--;
        showCurrentPage();
        updatePageNavigation();
    }
}

/**
 * @brief 下一页按钮点击处理
 */
void MainWindow::onNextPageClicked()
{
    if (m_currentPageIndex < m_loadedImages.size() - 1) {
        m_currentPageIndex++;
        showCurrentPage();
        updatePageNavigation();
    }
}

/**
 * @brief 复制结果按钮点击处理
 */
void MainWindow::onCopyResultClicked()
{
    if (!m_currentOCRResult.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(m_currentOCRResult);
        showStatusMessage("OCR结果已复制到剪贴板");
    }
}

/**
 * @brief 保存结果按钮点击处理
 */
void MainWindow::onSaveResultClicked()
{
    if (m_currentOCRResult.isEmpty()) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "保存OCR识别结果",
        "ocr_result.txt",
        "文本文件 (*.txt);;所有文件 (*.*)");

    if (!fileName.isEmpty()) {
        if (saveTextToFile(fileName, m_currentOCRResult)) {
            showStatusMessage("OCR结果已保存到: " + QFileInfo(fileName).fileName());
        } else {
            QMessageBox::warning(this, "错误", "保存文件失败");
        }
    }
}

/**
 * @brief 清空结果按钮点击处理
 */
void MainWindow::onClearResultClicked()
{
    ui->textEditResult->clear();
    m_currentOCRResult.clear();
    updateUIState(m_hasValidFile);
    showStatusMessage("OCR结果已清空");
}

// 菜单动作处理槽函数实现

/**
 * @brief 菜单-打开文件
 */
void MainWindow::onActionOpenFile()
{
    onSelectFileClicked();
}

/**
 * @brief 菜单-保存结果
 */
void MainWindow::onActionSaveResult()
{
    onSaveResultClicked();
}

/**
 * @brief 菜单-退出程序
 */
void MainWindow::onActionExit()
{
    close();
}

/**
 * @brief 菜单-复制结果
 */
void MainWindow::onActionCopyResult()
{
    onCopyResultClicked();
}

/**
 * @brief 菜单-清空结果
 */
void MainWindow::onActionClearResult()
{
    onClearResultClicked();
}

/**
 * @brief 菜单-OCR设置
 */
void MainWindow::onActionOCRSettings()
{
    QMessageBox::information(this, "OCR设置", "OCR设置功能将在后续版本中实现");
}

/**
 * @brief 菜单-语言设置
 */
void MainWindow::onActionLanguageSettings()
{
    QMessageBox::information(this, "语言设置", "语言设置功能将在后续版本中实现");
}

/**
 * @brief 菜单-关于
 */
void MainWindow::onActionAbout()
{
    QMessageBox::about(this, "关于智能OCR文字识别工具",
                      "<h2>智能OCR文字识别工具 v1.0</h2>"
                      "<p>基于Qt和Tesseract OCR开发的文字识别应用程序</p>"
                      "<p>支持多种图像格式和PDF文档的文字识别</p>"
                      "<p><b>主要功能：</b></p>"
                      "<ul>"
                      "<li>支持PNG、JPG、BMP、TIFF、WebP、PDF等格式</li>"
                      "<li>多语言文字识别（中文简体、中文繁体、英文等）</li>"
                      "<li>批量导入，批量处理</li>"
                      "<li>快速截取屏幕内容进行识别</li>"
                      "</ul>");
}

/**
 * @brief 菜单-帮助
 */
void MainWindow::onActionHelp()
{
    QMessageBox::information(this, "使用帮助",
                           "<h3>使用方法：</h3>"
                           "<ol>"
                           "<li><b>选择文件：</b> 点击\"选择文件\"按钮或使用菜单\"文件->打开文件\"选择要识别的图像或PDF文件</li>"
                           "<li><b>设置语言：</b> 在左侧面板选择识别语言（默认为中英混合）</li>"
                           "<li><b>开始识别：</b> 点击\"开始识别\"按钮开始OCR文字识别</li>"
                           "<li><b>查看结果：</b> 识别完成后，结果会显示在\"识别结果\"标签页中</li>"
                           "<li><b>导出结果：</b> 可以复制结果或保存到文件</li>"
                           "</ol>"
                           "<h3>支持的文件格式：</h3>"
                           "<p>图像格式：PNG, JPG, JPEG, BMP, GIF, TIFF, WebP</p>"
                           "<p>文档格式：PDF（需要安装PDF转换工具）</p>"
                           "<h3>注意事项：</h3>"
                           "<p>• 确保图像质量清晰，文字对比度良好</p>"
                           "<p>• 建议图像分辨率不低于150 DPI</p>"
                           "<p>• PDF转换功能需要安装Poppler Utils</p>");
}

/**
 * @brief 菜单-开源协议
 */
void MainWindow::onActionLicense()
{
    LicenseDialog dialog(this);
    dialog.exec();
}

// 文件处理相关槽函数实现

/**
 * @brief 文件处理进度更新
 * @param progress 进度百分比
 * @param currentPage 当前页面
 * @param totalPages 总页面数
 */
void MainWindow::onFileProcessProgress(int progress, int currentPage, int totalPages)
{
    ui->progressBar->setValue(progress);
    m_statusProgressBar->setValue(progress);

    if (totalPages > 1) {
        ui->lblProgressText->setText(QString("正在处理第 %1/%2 页...").arg(currentPage).arg(totalPages));
    } else {
        ui->lblProgressText->setText("正在处理文件...");
    }
}

/**
 * @brief 文件处理完成
 * @param result 处理结果
 */
void MainWindow::onFileProcessCompleted(const FileProcessor::ProcessResult &result)
{
    m_isProcessing = false;
    ui->progressBar->setValue(100);
    m_statusProgressBar->setVisible(false);

    if (result.success && !result.images.isEmpty()) {
        m_loadedImages = result.images;
        m_imageNames = result.pageNames;
        m_currentPageIndex = 0;

        showCurrentPage();
        updateUIState(true);

        QString message = QString("成功加载 %1 个页面").arg(result.pageCount);
        ui->lblProgressText->setText(message);
        showStatusMessage(message);

        // 如果只有一页，自动切换到图像预览
        ui->tabWidget->setCurrentIndex(0);
    } else {
        updateUIState(false);
        ui->lblProgressText->setText("文件加载失败");
        showStatusMessage("错误: " + result.errorMessage);

        QMessageBox::warning(this, "文件处理失败", result.errorMessage);
    }
}

/**
 * @brief 文件处理错误
 * @param errorMessage 错误信息
 */
void MainWindow::onFileProcessError(const QString &errorMessage)
{
    m_isProcessing = false;
    ui->progressBar->setValue(0);
    m_statusProgressBar->setVisible(false);

    updateUIState(false);
    ui->lblProgressText->setText("处理失败");
    showStatusMessage("错误: " + errorMessage);

    QMessageBox::critical(this, "文件处理错误", errorMessage);
}

// OCR引擎相关槽函数实现

/**
 * @brief OCR处理进度更新
 * @param progress 进度百分比
 */
void MainWindow::onOCRProgress(int progress)
{
    ui->progressBar->setValue(progress);
    m_statusProgressBar->setValue(progress);
}

/**
 * @brief OCR处理完成
 * @param result OCR结果
 */
void MainWindow::onOCRCompleted(const OCREngine::OCRResult &result)
{
    m_isProcessing = false;
    ui->progressBar->setValue(100);
    m_statusProgressBar->setVisible(false);

    if (result.success) {
        m_currentOCRResult = result.text;
        ui->textEditResult->setPlainText(result.text);

        QString message = QString("OCR识别完成，置信度: %1%").arg(QString::number(result.confidence * 100, 'f', 1));
        ui->lblProgressText->setText(message);
        showStatusMessage(message);

        // 自动切换到结果标签页
        ui->tabWidget->setCurrentIndex(1);
    } else {
        ui->lblProgressText->setText("OCR识别失败");
        showStatusMessage("OCR错误: " + result.errorMessage);
        QMessageBox::warning(this, "OCR识别失败", result.errorMessage);
    }

    updateUIState(m_hasValidFile);
}

/**
 * @brief OCR处理错误
 * @param errorMessage 错误信息
 */
void MainWindow::onOCRError(const QString &errorMessage)
{
    m_isProcessing = false;
    ui->progressBar->setValue(0);
    m_statusProgressBar->setVisible(false);

    ui->lblProgressText->setText("OCR识别失败");
    showStatusMessage("OCR错误: " + errorMessage);
    updateUIState(m_hasValidFile);

    QMessageBox::critical(this, "OCR识别错误", errorMessage);
}

/**
 * @brief 批量OCR处理进度更新
 * @param progress 整体进度百分比
 * @param currentPage 当前处理页面
 * @param totalPages 总页面数
 * @param currentPageProgress 当前页面进度
 */
void MainWindow::onBatchOCRProgress(int progress, int currentPage, int totalPages, int currentPageProgress)
{
    ui->progressBar->setValue(progress);
    m_statusProgressBar->setValue(progress);

    QString progressText = QString("正在识别第 %1/%2 页... (%3%)")
                          .arg(currentPage)
                          .arg(totalPages)
                          .arg(progress);
    ui->lblProgressText->setText(progressText);

    QString statusMessage = QString("批量OCR进度: 第%1页 %2% (总体 %3%)")
                           .arg(currentPage)
                           .arg(currentPageProgress)
                           .arg(progress);
    showStatusMessage(statusMessage, 0);
}

/**
 * @brief 批量OCR处理完成
 * @param result 批量OCR结果
 */
void MainWindow::onBatchOCRCompleted(const OCREngine::BatchOCRResult &result)
{
    m_isProcessing = false;
    ui->progressBar->setValue(100);
    m_statusProgressBar->setVisible(false);

    if (result.success) {
        // 显示合并后的所有文本结果
        m_currentOCRResult = result.combinedText;
        ui->textEditResult->setPlainText(result.combinedText);

        QString message;
        if (result.processedPages == result.totalPages) {
            message = QString("批量OCR识别完成！成功处理 %1 页")
                     .arg(result.processedPages);
        } else {
            message = QString("批量OCR识别部分完成：成功处理 %1/%2 页")
                     .arg(result.processedPages)
                     .arg(result.totalPages);
        }

        ui->lblProgressText->setText(message);
        showStatusMessage(message);

        // 自动切换到结果标签页
        ui->tabWidget->setCurrentIndex(1);
    } else {
        ui->lblProgressText->setText("批量OCR识别失败");
        showStatusMessage("批量OCR错误: " + result.errorMessage);
        QMessageBox::warning(this, "批量OCR识别失败", result.errorMessage);
    }

    updateUIState(m_hasValidFile);
}

// 界面更新相关槽函数实现

/**
 * @brief 语言选择改变
 */
void MainWindow::onLanguageChanged()
{
    QString currentLanguage = ui->comboLanguage->currentText();
    QString languageCode = getCurrentLanguageCode();
    showStatusMessage("已选择语言: " + currentLanguage + " (" + languageCode + ")");

    // 保存用户的语言选择偏好
    saveLanguagePreference();
}

/**
 * @brief OCR引擎选择改变
 */
void MainWindow::onEngineChanged()
{
    QString engineName = ui->comboEngine->currentText();
    showStatusMessage("已选择OCR引擎: " + engineName);
}

// 辅助函数实现

/**
 * @brief 显示图像预览
 * @param image 要显示的图像
 */
void MainWindow::showImagePreview(const QImage &image)
{
    if (image.isNull()) {
        ui->lblImagePreview->setText("无法显示图像");
        ui->lblImagePreview->setPixmap(QPixmap());
        // 重置标签的大小限制
        ui->lblImagePreview->setMinimumSize(0, 0);
        ui->lblImagePreview->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        return;
    }

    // 获取滚动区域的viewport大小，这是实际可见的区域
    QSize viewportSize = ui->imageScrollArea->viewport()->size();

    // 为了避免滚动条，我们需要确保图像尺寸略小于viewport尺寸
    // 减去更多边距以确保不会出现滚动条
    QSize maxSize = viewportSize - QSize(40, 40);

    // 确保最小尺寸合理
    maxSize = maxSize.expandedTo(QSize(100, 100));

    // 计算缩放后的图像大小，保持宽高比
    QSize scaledSize = image.size();
    scaledSize.scale(maxSize, Qt::KeepAspectRatio);

    // 创建缩放后的图像，使用高质量缩放
    QPixmap pixmap = QPixmap::fromImage(image.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // 设置标签的最小和最大尺寸，确保标签尺寸与图像匹配
    ui->lblImagePreview->setMinimumSize(scaledSize);
    ui->lblImagePreview->setMaximumSize(scaledSize);

    // 显示图像
    ui->lblImagePreview->setPixmap(pixmap);
    ui->lblImagePreview->setAlignment(Qt::AlignCenter);
}

/**
 * @brief 更新页面导航
 */
void MainWindow::updatePageNavigation()
{
    if (m_loadedImages.isEmpty()) {
        return;
    }

    ui->btnPrevPage->setEnabled(m_currentPageIndex > 0);
    ui->btnNextPage->setEnabled(m_currentPageIndex < m_loadedImages.size() - 1);
    ui->lblPageInfo->setText(QString("第 %1 页 / 共 %2 页")
                           .arg(m_currentPageIndex + 1)
                           .arg(m_loadedImages.size()));
}

/**
 * @brief 显示当前页面
 */
void MainWindow::showCurrentPage()
{
    if (m_currentPageIndex >= 0 && m_currentPageIndex < m_loadedImages.size()) {
        showImagePreview(m_loadedImages[m_currentPageIndex]);
        updatePageNavigation();
    }
}

/**
 * @brief 获取当前选择的语言代码
 * @return 语言代码字符串
 */
QString MainWindow::getCurrentLanguageCode() const
{
    QString displayName = ui->comboLanguage->currentText();
    return s_languageCodeMap.value(displayName, "chi_sim+eng"); // 默认返回中英混合
}

/**
 * @brief 获取语言显示名称
 * @param languageCode 语言代码
 * @return 显示名称
 */
QString MainWindow::getLanguageDisplayName(const QString &languageCode) const
{
    return s_languageNameMap.value(languageCode, languageCode);
}

/**
 * @brief 显示状态栏消息
 * @param message 消息内容
 * @param timeout 超时时间（毫秒，0表示不自动消失）
 */
void MainWindow::showStatusMessage(const QString &message, int timeout)
{
    if (m_statusLabel) {
        m_statusLabel->setText(message);
        if (timeout > 0) {
            QTimer::singleShot(timeout, [this]() {
                m_statusLabel->setText("准备就绪");
            });
        }
    }
}

/**
 * @brief 保存OCR结果到文件
 * @param filePath 保存路径
 * @param content 要保存的内容
 * @return 是否成功
 */
bool MainWindow::saveTextToFile(const QString &filePath, const QString &content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;

    return true;
}

/**
 * @brief 开始屏幕截图
 */
void MainWindow::startScreenCapture()
{
    // 创建截图窗口
    ScreenCapture *screenCapture = new ScreenCapture();

    // 连接截图完成信号
    connect(screenCapture, &ScreenCapture::captureFinished,
            this, &MainWindow::onScreenCaptureFinished);

    // 开始截图
    if (!screenCapture->startCapture()) {
        delete screenCapture;
        // 如果窗口被隐藏了，需要先恢复显示再显示错误消息
        if (m_windowHiddenForCapture) {
            show();
            m_windowHiddenForCapture = false;
        }
        QMessageBox::warning(this, "错误", "无法启动屏幕截图功能");
    }
}

/**
 * @brief 保存语言偏好设置
 */
void MainWindow::saveLanguagePreference()
{
    QString selectedLanguage = ui->comboLanguage->currentText();
    QSettings settings;

    // 显示QSettings存储位置信息
    qDebug() << "QSettings文件路径:" << settings.fileName() << Qt::endl;

    settings.setValue("language/selectedLanguage", selectedLanguage);
    settings.sync(); // 强制同步写入到磁盘

    // 验证写入是否成功 - 重新读取确认
    QString verifyRead = settings.value("language/selectedLanguage").toString();
    qDebug() << "已保存语言设置:" << selectedLanguage << Qt::endl;
    qDebug() << "验证读取结果:" << verifyRead << Qt::endl;
    qDebug() << "写入验证:" << (selectedLanguage == verifyRead ? "成功" : "失败") << Qt::endl;
}

/**
 * @brief 加载语言偏好设置
 */
void MainWindow::loadLanguagePreference()
{
    QSettings settings;
    QString savedLanguage = settings.value("language/selectedLanguage", "中英混合").toString();

    // 查找并设置保存的语言选项
    int index = ui->comboLanguage->findText(savedLanguage);
    if (index != -1) {
        ui->comboLanguage->setCurrentIndex(index);
    } else {
        // 如果找不到保存的语言，使用默认的"中英混合"
        ui->comboLanguage->setCurrentText("中英混合");
    }
}

/**
 * @brief 窗口大小改变事件处理
 * @param event 大小改变事件
 */
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // 如果有加载的图像，并且当前正在显示图像预览，则重新调整图像大小
    if (!m_loadedImages.isEmpty() && m_currentPageIndex >= 0 && m_currentPageIndex < m_loadedImages.size()) {
        // 使用定时器延迟执行，确保界面布局已经完成
        QTimer::singleShot(50, [this]() {
            if (!m_loadedImages.isEmpty() && m_currentPageIndex >= 0 && m_currentPageIndex < m_loadedImages.size()) {
                showImagePreview(m_loadedImages[m_currentPageIndex]);
            }
        });
    }
}
