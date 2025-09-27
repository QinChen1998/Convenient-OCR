#include "ocrengine.h"

/**
 * @brief OCREngine构造函数
 * @param parent 父对象指针
 */
OCREngine::OCREngine(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
    // 基类构造函数，初始化成员变量
}