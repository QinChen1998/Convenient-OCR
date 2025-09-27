#ifndef LICENSEDIALOG_H
#define LICENSEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class LicenseDialog;
}
QT_END_NAMESPACE

/**
 * @brief 开源协议介绍对话框
 *
 * 该类用于显示项目的开源协议信息，包括项目本身的协议
 * 以及所使用的第三方依赖库的协议信息。
 */
class LicenseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LicenseDialog(QWidget *parent = nullptr);
    ~LicenseDialog();

private:
    Ui::LicenseDialog *ui;

    /**
     * @brief 初始化协议内容
     */
    void initLicenseContent();
};

#endif // LICENSEDIALOG_H