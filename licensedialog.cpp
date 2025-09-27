#include "licensedialog.h"
#include "ui_licensedialog.h"

LicenseDialog::LicenseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LicenseDialog)
{
    ui->setupUi(this);
    initLicenseContent();
}

LicenseDialog::~LicenseDialog()
{
    delete ui;
}

void LicenseDialog::initLicenseContent()
{
    QString licenseContent = QString(
        "许可证 (License) \n"
        "本项目采用GNU General Public License v2.0 (GPL v2.0)协议发布。 \n"
        "有关完整条款，请参阅项目根目录下的LICENSE文件。\n \n"
        "第三方依赖库\n"
        "本项目使用了多个第三方库，包括 Poppler (GPL v2)、Qt (LGPL v3) 和 Tesseract (Apache 2.0) 等。\n"
    );

    ui->textBrowserLicense->setText(licenseContent);
}
