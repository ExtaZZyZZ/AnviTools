#pragma once
#include <cstdint>
#include <QtCore/QString>
#include <QWidget>

class QLabel;

class ModulePlaceholderTab : public QWidget {
    Q_OBJECT
public:
    explicit ModulePlaceholderTab(const QString& title,
                                  const QString& description,
                                  QWidget* parent = nullptr);

    void setModuleConnected(bool connected, uint8_t nodeId);

private:
    QLabel* statusBadge_ = nullptr;
    QLabel* statusHint_  = nullptr;
    QLabel* noteLabel_   = nullptr;
    QString description_;
};
