#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QTextStream>
#include <QScrollBar>
#include <QTimer>
#include <QSettings>
#include <QResizeEvent>

// 引入自定义类
#include "ocrengine.h"
#include "tesseractocrengine.h"
#include "fileprocessor.h"
#include "screencapture.h"
#include "licensedialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @brief 主窗口类
 *
 * 该类是应用程序的主界面，负责协调各个组件之间的交互，
 * 包括文件处理、OCR识别、结果显示等功能。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    /**
     * @brief 窗口大小改变事件处理
     * @param event 大小改变事件
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // UI事件处理槽函数

    /**
     * @brief 选择文件按钮点击处理
     */
    void onSelectFileClicked();

    /**
     * @brief 屏幕截图按钮点击处理
     */
    void onScreenCaptureClicked();

    /**
     * @brief 开始OCR识别按钮点击处理
     */
    void onStartOCRClicked();

    /**
     * @brief 上一页按钮点击处理
     */
    void onPrevPageClicked();

    /**
     * @brief 下一页按钮点击处理
     */
    void onNextPageClicked();

    /**
     * @brief 复制结果按钮点击处理
     */
    void onCopyResultClicked();

    /**
     * @brief 保存结果按钮点击处理
     */
    void onSaveResultClicked();

    /**
     * @brief 清空结果按钮点击处理
     */
    void onClearResultClicked();

    // 菜单动作处理槽函数

    /**
     * @brief 菜单-打开文件
     */
    void onActionOpenFile();

    /**
     * @brief 菜单-保存结果
     */
    void onActionSaveResult();

    /**
     * @brief 菜单-退出程序
     */
    void onActionExit();

    /**
     * @brief 菜单-复制结果
     */
    void onActionCopyResult();

    /**
     * @brief 菜单-清空结果
     */
    void onActionClearResult();

    /**
     * @brief 菜单-OCR设置
     */
    void onActionOCRSettings();

    /**
     * @brief 菜单-语言设置
     */
    void onActionLanguageSettings();

    /**
     * @brief 菜单-关于
     */
    void onActionAbout();

    /**
     * @brief 菜单-帮助
     */
    void onActionHelp();

    /**
     * @brief 菜单-开源协议
     */
    void onActionLicense();

    // 文件处理相关槽函数

    /**
     * @brief 文件处理进度更新
     * @param progress 进度百分比
     * @param currentPage 当前页面
     * @param totalPages 总页面数
     */
    void onFileProcessProgress(int progress, int currentPage, int totalPages);

    /**
     * @brief 文件处理完成
     * @param result 处理结果
     */
    void onFileProcessCompleted(const FileProcessor::ProcessResult &result);

    /**
     * @brief 文件处理错误
     * @param errorMessage 错误信息
     */
    void onFileProcessError(const QString &errorMessage);

    /**
     * @brief 截图完成处理
     * @param imagePath 截图文件路径，空字符串表示取消
     */
    void onScreenCaptureFinished(const QString &imagePath);

    // OCR引擎相关槽函数

    /**
     * @brief OCR处理进度更新
     * @param progress 进度百分比
     */
    void onOCRProgress(int progress);

    /**
     * @brief 批量OCR处理进度更新
     * @param progress 整体进度百分比
     * @param currentPage 当前处理页面
     * @param totalPages 总页面数
     * @param currentPageProgress 当前页面进度
     */
    void onBatchOCRProgress(int progress, int currentPage, int totalPages, int currentPageProgress);

    /**
     * @brief OCR处理完成
     * @param result OCR结果
     */
    void onOCRCompleted(const OCREngine::OCRResult &result);

    /**
     * @brief 批量OCR处理完成
     * @param result 批量OCR结果
     */
    void onBatchOCRCompleted(const OCREngine::BatchOCRResult &result);

    /**
     * @brief OCR处理错误
     * @param errorMessage 错误信息
     */
    void onOCRError(const QString &errorMessage);

    // 界面更新相关槽函数

    /**
     * @brief 语言选择改变
     */
    void onLanguageChanged();

    /**
     * @brief OCR引擎选择改变
     */
    void onEngineChanged();

private:
    /**
     * @brief 初始化UI组件
     */
    void initUI();

    /**
     * @brief 连接信号和槽
     */
    void connectSignalsAndSlots();

    /**
     * @brief 初始化OCR引擎
     */
    void initOCREngine();

    /**
     * @brief 更新UI状态
     * @param hasFile 是否已选择文件
     */
    void updateUIState(bool hasFile = false);

    /**
     * @brief 显示图像预览
     * @param image 要显示的图像
     */
    void showImagePreview(const QImage &image);

    /**
     * @brief 更新页面导航
     */
    void updatePageNavigation();

    /**
     * @brief 显示当前页面
     */
    void showCurrentPage();

    /**
     * @brief 获取当前选择的语言代码
     * @return 语言代码字符串
     */
    QString getCurrentLanguageCode() const;

    /**
     * @brief 获取语言显示名称
     * @param languageCode 语言代码
     * @return 显示名称
     */
    QString getLanguageDisplayName(const QString &languageCode) const;

    /**
     * @brief 显示状态栏消息
     * @param message 消息内容
     * @param timeout 超时时间（毫秒）
     */
    void showStatusMessage(const QString &message, int timeout = 3000);

    /**
     * @brief 保存OCR结果到文件
     * @param filePath 保存路径
     * @param content 要保存的内容
     * @return 是否成功
     */
    bool saveTextToFile(const QString &filePath, const QString &content);

    /**
     * @brief 开始屏幕截图
     */
    void startScreenCapture();

    /**
     * @brief 保存语言偏好设置
     */
    void saveLanguagePreference();

    /**
     * @brief 加载语言偏好设置
     */
    void loadLanguagePreference();

private:
    Ui::MainWindow *ui;                        // UI对象指针

    // 核心功能组件
    FileProcessor *m_fileProcessor;            // 文件处理器
    OCREngine *m_ocrEngine;                   // OCR引擎（当前使用的）
    TesseractOCREngine *m_tesseractEngine;    // Tesseract OCR引擎

    // 数据存储
    QList<QImage> m_loadedImages;             // 加载的图像列表
    QStringList m_imageNames;                 // 图像名称列表
    QString m_currentFilePath;                // 当前文件路径
    QString m_currentOCRResult;               // 当前OCR识别结果

    // UI状态管理
    int m_currentPageIndex;                   // 当前页面索引
    bool m_isProcessing;                      // 是否正在处理
    bool m_hasValidFile;                      // 是否有有效文件
    bool m_windowHiddenForCapture;            // 是否因截图而隐藏窗口

    // 状态栏标签
    QLabel *m_statusLabel;                    // 状态栏信息标签
    QProgressBar *m_statusProgressBar;        // 状态栏进度条

    // 语言代码映射
    static const QMap<QString, QString> s_languageCodeMap;  // 显示名称到代码的映射
    static const QMap<QString, QString> s_languageNameMap;  // 代码到显示名称的映射
};

#endif // MAINWINDOW_H
