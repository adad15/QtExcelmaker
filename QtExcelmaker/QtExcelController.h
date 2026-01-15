#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include "ExcelPipeline.h"

class QtExcelController final : public QObject {
    Q_OBJECT
public:
    explicit QtExcelController(std::unique_ptr<core::IExcelReader> reader,
        std::unique_ptr<core::IExcelWriter> writer,
        QObject* parent = nullptr);

    struct UiInput {
        QString mixedFolder;
        QString roadFolder;
        QString facilityFolder;
        QString roadTemplate;
        QString facilityTemplate;
        QString outputDir;
        bool includeSubfolders = true;
    };

    void run(const UiInput& input);

signals:
    void progressTextChanged(const QString& text);
    void progressUpdated(int stage, int current, int total, const QString& message);
    void finished(bool ok, const QString& summary);

private:
    std::unique_ptr<core::IExcelReader> reader_;
    std::unique_ptr<core::IExcelWriter> writer_;
    std::unique_ptr<core::ExcelPipeline> pipeline_;
};
