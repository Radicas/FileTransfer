/**
 * @file transferprogresspanel.cpp
 * @brief 传输进度面板控件实现
 */

#include "ui/widgets/transferprogresspanel.h"
#include "common/logger/logger.h"
#include <QDateTime>

TransferProgressPanel::TransferProgressPanel(QWidget* parent)
    : QWidget(parent)
    , m_transferTable(nullptr)
    , m_titleLabel(nullptr)
    , m_clearButton(nullptr)
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
{
    initUI();

    connect(&FileTransferService::instance(), &FileTransferService::transferProgress,
            this, &TransferProgressPanel::onTransferProgress);
    connect(&FileTransferService::instance(), &FileTransferService::transferCompleted,
            this, &TransferProgressPanel::onTransferCompleted);
    connect(&FileTransferService::instance(), &FileTransferService::transferStateChanged,
            this, &TransferProgressPanel::onTransferStateChanged);

    Logger::instance().info("传输进度面板初始化完成");
}

TransferProgressPanel::~TransferProgressPanel()
{
}

int TransferProgressPanel::getActiveTransferCount() const
{
    return FileTransferService::instance().getActiveTransferCount();
}

void TransferProgressPanel::clearCompletedTransfers()
{
    QList<TransferTaskInfo> tasks = FileTransferService::instance().getAllTransfers();
    
    for (const TransferTaskInfo& task : tasks)
    {
        if (task.state == TransferState::Completed || task.state == TransferState::Failed || task.state == TransferState::Cancelled)
        {
            int row = findTransferRow(task.taskId);
            if (row >= 0)
            {
                m_transferTable->removeRow(row);
                m_taskRowMap.remove(task.taskId);
            }
        }
    }
}

void TransferProgressPanel::clearAllTransfers()
{
    m_transferTable->setRowCount(0);
    m_taskRowMap.clear();
}

void TransferProgressPanel::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);

    m_headerLayout = new QHBoxLayout();
    m_headerLayout->setSpacing(10);

    m_titleLabel = new QLabel("传输任务", this);
    m_titleLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    m_headerLayout->addWidget(m_titleLabel);

    m_clearButton = new QPushButton("清除已完成", this);
    m_clearButton->setFixedSize(100, 25);
    m_headerLayout->addWidget(m_clearButton);

    m_headerLayout->addStretch();
    m_mainLayout->addLayout(m_headerLayout);

    m_transferTable = new QTableWidget(this);
    m_transferTable->setColumnCount(6);
    m_transferTable->setHorizontalHeaderLabels({"文件", "进度", "大小", "速度", "状态", "操作"});
    m_transferTable->horizontalHeader()->setStretchLastSection(true);
    m_transferTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_transferTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_transferTable->verticalHeader()->setVisible(false);
    m_transferTable->setAlternatingRowColors(true);
    
    m_transferTable->setColumnWidth(0, 200);
    m_transferTable->setColumnWidth(1, 150);
    m_transferTable->setColumnWidth(2, 100);
    m_transferTable->setColumnWidth(3, 100);
    m_transferTable->setColumnWidth(4, 80);
    m_transferTable->setColumnWidth(5, 80);

    m_mainLayout->addWidget(m_transferTable);

    connect(m_clearButton, &QPushButton::clicked, this, &TransferProgressPanel::clearCompletedTransfers);
}

void TransferProgressPanel::onTransferProgress(const QString& taskId, qint64 transferredSize, qint64 totalSize, int progress)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    if (m_lastUpdateTime.contains(taskId))
    {
        qint64 elapsed = currentTime - m_lastUpdateTime[taskId];
        qint64 transferredDiff = transferredSize - m_lastTransferred[taskId];
        
        if (elapsed > 0)
        {
            m_transferSpeed[taskId] = (transferredDiff * 1000) / elapsed;
        }
    }
    
    m_lastUpdateTime[taskId] = currentTime;
    m_lastTransferred[taskId] = transferredSize;
    
    updateTransferRow(taskId);
}

void TransferProgressPanel::onTransferCompleted(const QString& taskId, bool success, const QString& errorMessage)
{
    updateTransferRow(taskId);
    
    TransferTaskInfo task = FileTransferService::instance().getTransferInfo(taskId);
    QString fileName = QFileInfo(task.sourcePath).fileName();
    
    if (success)
    {
        Logger::instance().info("传输完成: " + fileName);
    }
    else
    {
        Logger::instance().error("传输失败: " + fileName + " - " + errorMessage);
    }
    
    emit transferCountChanged(getActiveTransferCount());
}

void TransferProgressPanel::onTransferStateChanged(const QString& taskId, TransferState state)
{
    if (state == TransferState::Running || state == TransferState::Preparing)
    {
        TransferTaskInfo task = FileTransferService::instance().getTransferInfo(taskId);
        if (findTransferRow(taskId) < 0)
        {
            addTransferRow(task);
        }
    }
    
    updateTransferRow(taskId);
    emit transferCountChanged(getActiveTransferCount());
}

void TransferProgressPanel::onCancelClicked()
{
    int row = m_transferTable->currentRow();
    if (row >= 0)
    {
        QString taskId = m_taskRowMap.keys().value(row);
        if (!taskId.isEmpty())
        {
            FileTransferService::instance().cancelTransfer(taskId);
            Logger::instance().info("取消传输任务: " + taskId);
        }
    }
}

void TransferProgressPanel::onClearClicked()
{
    clearCompletedTransfers();
}

void TransferProgressPanel::addTransferRow(const TransferTaskInfo& taskInfo)
{
    int row = m_transferTable->rowCount();
    m_transferTable->insertRow(row);
    m_taskRowMap.insert(taskInfo.taskId, row);

    QString fileName = QFileInfo(taskInfo.sourcePath).fileName();
    
    QTableWidgetItem* nameItem = new QTableWidgetItem(fileName);
    nameItem->setToolTip(taskInfo.sourcePath);
    m_transferTable->setItem(row, 0, nameItem);

    QProgressBar* progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(taskInfo.progress);
    progressBar->setTextVisible(true);
    progressBar->setFormat("%p%");
    m_transferTable->setCellWidget(row, 1, progressBar);

    QTableWidgetItem* sizeItem = new QTableWidgetItem(formatSize(taskInfo.totalSize));
    m_transferTable->setItem(row, 2, sizeItem);

    QTableWidgetItem* speedItem = new QTableWidgetItem("--");
    m_transferTable->setItem(row, 3, speedItem);

    QTableWidgetItem* stateItem = new QTableWidgetItem(getStateText(taskInfo.state));
    stateItem->setForeground(getStateColor(taskInfo.state));
    m_transferTable->setItem(row, 4, stateItem);

    QPushButton* cancelButton = new QPushButton("取消", this);
    cancelButton->setFixedSize(60, 25);
    connect(cancelButton, &QPushButton::clicked, [this, taskInfo]() {
        FileTransferService::instance().cancelTransfer(taskInfo.taskId);
    });
    m_transferTable->setCellWidget(row, 5, cancelButton);
}

void TransferProgressPanel::updateTransferRow(const QString& taskId)
{
    int row = findTransferRow(taskId);
    if (row < 0)
    {
        return;
    }

    TransferTaskInfo taskInfo = FileTransferService::instance().getTransferInfo(taskId);

    QProgressBar* progressBar = qobject_cast<QProgressBar*>(m_transferTable->cellWidget(row, 1));
    if (progressBar)
    {
        progressBar->setValue(taskInfo.progress);
    }

    QTableWidgetItem* speedItem = m_transferTable->item(row, 3);
    if (speedItem && m_transferSpeed.contains(taskId))
    {
        speedItem->setText(formatSpeed(m_transferSpeed[taskId]));
    }

    QTableWidgetItem* stateItem = m_transferTable->item(row, 4);
    if (stateItem)
    {
        stateItem->setText(getStateText(taskInfo.state));
        stateItem->setForeground(getStateColor(taskInfo.state));
    }

    QPushButton* cancelButton = qobject_cast<QPushButton*>(m_transferTable->cellWidget(row, 5));
    if (cancelButton)
    {
        bool canCancel = taskInfo.state == TransferState::Running || 
                         taskInfo.state == TransferState::Preparing || 
                         taskInfo.state == TransferState::Pending;
        cancelButton->setEnabled(canCancel);
        cancelButton->setText(canCancel ? "取消" : "--");
    }
}

int TransferProgressPanel::findTransferRow(const QString& taskId) const
{
    return m_taskRowMap.value(taskId, -1);
}

QString TransferProgressPanel::getStateText(TransferState state) const
{
    switch (state)
    {
    case TransferState::Pending:
        return "等待中";
    case TransferState::Preparing:
        return "准备中";
    case TransferState::Running:
        return "传输中";
    case TransferState::Paused:
        return "已暂停";
    case TransferState::Completed:
        return "已完成";
    case TransferState::Failed:
        return "失败";
    case TransferState::Cancelled:
        return "已取消";
    default:
        return "未知";
    }
}

QColor TransferProgressPanel::getStateColor(TransferState state) const
{
    switch (state)
    {
    case TransferState::Running:
        return QColor(0, 120, 215);
    case TransferState::Completed:
        return QColor(0, 150, 0);
    case TransferState::Failed:
        return QColor(200, 0, 0);
    case TransferState::Cancelled:
        return QColor(150, 150, 0);
    default:
        return QColor(100, 100, 100);
    }
}

QString TransferProgressPanel::formatSize(qint64 bytes) const
{
    if (bytes < 1024)
    {
        return QString::number(bytes) + " B";
    }
    else if (bytes < 1024 * 1024)
    {
        return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    }
    else if (bytes < 1024 * 1024 * 1024)
    {
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    }
    else
    {
        return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GB";
    }
}

QString TransferProgressPanel::formatSpeed(qint64 bytesPerSecond) const
{
    if (bytesPerSecond < 1024)
    {
        return QString::number(bytesPerSecond) + " B/s";
    }
    else if (bytesPerSecond < 1024 * 1024)
    {
        return QString::number(bytesPerSecond / 1024.0, 'f', 1) + " KB/s";
    }
    else
    {
        return QString::number(bytesPerSecond / (1024.0 * 1024.0), 'f', 1) + " MB/s";
    }
}