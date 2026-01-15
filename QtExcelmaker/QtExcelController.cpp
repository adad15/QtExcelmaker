#include "QtExcelController.h"

#include <QtConcurrent/QtConcurrent>

namespace {

    std::filesystem::path toPath(const QString& q) {
        return std::filesystem::path(q.toStdWString());
    }

    QString buildSummary(const core::Result& result) {
        return QString("ok=%1 files=%2 routes=%3 rows=%4 warnings=%5 errors=%6")
            .arg(result.ok())
            .arg(result.filesProcessed)
            .arg(result.routesWritten)
            .arg(result.rowsWritten)
            .arg(result.warnings.size())
            .arg(result.errors.size());
    }

}  // namespace

QtExcelController::QtExcelController(std::unique_ptr<core::IExcelReader> reader,
    std::unique_ptr<core::IExcelWriter> writer,
    QObject* parent)
    : QObject(parent),
    reader_(std::move(reader)),
    writer_(std::move(writer)) {
    pipeline_ = std::make_unique<core::ExcelPipeline>(
        *reader_, *writer_, [this](const core::ProgressEvent& e) {
            emit progressTextChanged(QString::fromStdString(e.message));
            emit progressUpdated(static_cast<int>(e.stage),
                static_cast<int>(e.current),
                static_cast<int>(e.total),
                QString::fromStdString(e.message));
        });
}

void QtExcelController::run(const UiInput& input) {
    core::ProcessConfig cfg;
    cfg.mixedFolder = toPath(input.mixedFolder);
    cfg.roadFolder = toPath(input.roadFolder);
    cfg.facilityFolder = toPath(input.facilityFolder);
    cfg.roadTemplate = toPath(input.roadTemplate);
    cfg.facilityTemplate = toPath(input.facilityTemplate);
    cfg.outputDir = toPath(input.outputDir);
    cfg.includeSubfolders = input.includeSubfolders;

    cfg.routeNameHeader = core::u8ToString(u8"\u7ebf\u8def\u540d\u79f0");
    cfg.roadKeywords = { core::u8ToString(u8"\u8def\u57fa") };
    cfg.facilityKeywords = { core::u8ToString(u8"\u8bbe\u65bd") };

    auto* pipeline = pipeline_.get();
    auto future = QtConcurrent::run([this, cfg, pipeline]() {
        core::setLogSink([this](const std::string& message) {
            emit progressTextChanged(QString::fromStdString(message));
        });
        core::Result result;
        try {
            result = pipeline->run(cfg);
        }
        catch (const std::exception& ex) {
            result.errors.push_back(std::string("unhandled exception: ") + ex.what());
        }
        catch (...) {
            result.errors.push_back("unhandled non-std exception");
        }

        for (const auto& warning : result.warnings) {
            emit progressTextChanged(QString::fromStdString("warning: " + warning));
        }
        for (const auto& error : result.errors) {
            emit progressTextChanged(QString::fromStdString("error: " + error));
        }
        core::setLogSink(nullptr);
        emit finished(result.ok(), buildSummary(result));
        });
    (void)future;
}
