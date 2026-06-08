/**
 * @file transferprogresspanel.h
 * @brief 传输进度面板控件定义
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 显示文件传输任务的进度和状态。
 */

#ifndef TRANSFERPROGRESSPANEL_H
#define TRANSFERPROGRESSPANEL_H

#include <QWidget>
#include <QTableWidget>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include "core/network/filetransferservice.h"

/**
 * @class TransferProgressPanel
 * @brief 传输进度面板控件
 * 
 * @details 该控件负责：
 * - 显示所有传输任务的列表
 * - 显示每个任务的进度条
 * - 提供取消传输功能
 * - 显示传输速度和剩余时间
 */
class TransferProgressPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit TransferProgressPanel(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~TransferProgressPanel();

    /**
     * @brief 获取正在进行的传输数量
     * @return 传输数量
     */
    int getActiveTransferCount() const;

    /**
     * @brief 清除已完成的传输
     */
    void clearCompletedTransfers();

    /**
     * @brief 清除所有传输
     */
    void clearAllTransfers();

signals:
    /**
     * @brief 传输数量变化信号
     * @param count 新的传输数量
     */
    void transferCountChanged(int count);

private slots:
    /**
     * @brief 处理传输进度更新
     * @param taskId 任务ID
     * @param transferredSize 已传输大小
     * @param totalSize 总大小
     * @param progress 进度百分比
     */
    void onTransferProgress(const QString& taskId, qint64 transferredSize, qint64 totalSize, int progress);

    /**
     * @brief 处理传输完成
     * @param taskId 任务ID
     * @param success 是否成功
     * @param errorMessage 错误消息
     */
    void onTransferCompleted(const QString& taskId, bool success, const QString& errorMessage);

    /**
     * @brief 处理传输状态变化
     * @param taskId 任务ID
     * @param state 新状态
     */
    void onTransferStateChanged(const QString& taskId, TransferState state);

    /**
     * @brief 处理取消按钮点击
     */
    void onCancelClicked();

    /**
     * @brief 处理清除按钮点击
     */
    void onClearClicked();

private:
    /**
     * @brief 初始化UI
     */
    void initUI();

    /**
     * @brief 添加传输任务行
     * @param taskInfo 任务信息
     */
    void addTransferRow(const TransferTaskInfo& taskInfo);

    /**
     * @brief 更新传输任务行
     * @param taskId 任务ID
     */
    void updateTransferRow(const QString& taskId);

    /**
     * @brief 查找传输任务行
     * @param taskId 任务ID
     * @return 行号，如果不存在返回-1
     */
    int findTransferRow(const QString& taskId) const;

    /**
     * @brief 获取状态显示文本
     * @param state 状态
     * @return 状态文本
     */
    QString getStateText(TransferState state) const;

    /**
     * @brief 获取状态颜色
     * @param state 状态
     * @return 颜色
     */
    QColor getStateColor(TransferState state) const;

    /**
     * @brief 格式化文件大小
     * @param bytes 字节数
     * @return 格式化后的字符串
     */
    QString formatSize(qint64 bytes) const;

    /**
     * @brief 格式化传输速度
     * @param bytesPerSecond 每秒字节数
     * @return 格式化后的字符串
     */
    QString formatSpeed(qint64 bytesPerSecond) const;

    QTableWidget* m_transferTable;        ///< 传输任务表格
    QLabel* m_titleLabel;                 ///< 标题标签
    QPushButton* m_clearButton;           ///< 清除按钮
    QVBoxLayout* m_mainLayout;            ///< 主布局
    QHBoxLayout* m_headerLayout;          ///< 头部布局

    QMap<QString, int> m_taskRowMap;      ///< 任务ID到行号的映射
    QMap<QString, qint64> m_lastUpdateTime; ///< 最后更新时间
    QMap<QString, qint64> m_lastTransferred; ///< 最后传输大小
    QMap<QString, qint64> m_transferSpeed;   ///< 传输速度
};

#endif  // TRANSFERPROGRESSPANEL_H