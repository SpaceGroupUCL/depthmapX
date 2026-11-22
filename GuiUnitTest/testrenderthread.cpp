// Copyright (C) 2011-2026, Tasos Varoudis
// This file is part of depthmapX and is licensed under GPL-3.0-or-later.

#include <catch.hpp>

#include <QEventLoop>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>

#include <functional>
#include <stdexcept>
#include <vector>

#include "mainwindow.h"

namespace {
class DefaultSettings : public Settings {
  public:
    const QVariant readSetting(const QString &, const QVariant &defaultValue) const override { return defaultValue; }
    void writeSetting(const QString &, const QVariant &) override {}
    std::unique_ptr<SettingsTransaction> getTransaction() override { return std::make_unique<DefaultSettings>(); }
};

class TestAnalysis : public IAnalysis {
    std::function<bool()> m_run;

  public:
    explicit TestAnalysis(std::function<bool()> run) : m_run(std::move(run)) {}
    std::string getAnalysisName() const override { return "Test analysis"; }
    bool run(Communicator *) override { return m_run(); }
};
} // namespace

TEST_CASE("The document recovers from a throwing module analysis", "[renderthread]") {
    DefaultSettings settings;
    MainWindow window(QString{}, settings);
    QWidget mapView;
    std::unique_ptr<MetaGraph> graph;
    QGraphDoc doc(QString{}, QString{});
    graph.reset(doc.m_meta_graph);
    doc.m_mainFrame = &window;
    doc.m_view[QGraphDoc::VIEW_MAP] = &mapView;

    std::vector<QString> messages;
    bool busyAtWarning = false;
    bool progressVisibleAtWarning = false;
    auto runAnalysis = [&](std::function<bool()> analysis) {
        REQUIRE(doc.m_communicator == nullptr);
        auto communicator = std::make_unique<CMSCommunicator>();
        communicator->SetFunction(CMSCommunicator::FROMCONNECTOR);
        communicator->setSuccessUpdateFlags(QGraphDoc::NEW_DATA);
        communicator->setSuccessRedrawFlags(QGraphDoc::VIEW_MAP, QGraphDoc::REDRAW_TOTAL, QGraphDoc::NEW_DATA);
        if (analysis) {
            communicator->setAnalysis(std::make_unique<TestAnalysis>(std::move(analysis)));
        }
        doc.m_communicator = communicator.release();
        doc.CreateWaitDialog("Test analysis");
        QPointer<QProgressDialog> progress = doc.m_waitdlg;
        REQUIRE(progress->isVisible());

        QEventLoop loop;
        QTimer poll;
        QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
            if (auto *warning = doc.findChild<QMessageBox *>()) {
                if (warning->isVisible()) {
                    messages.push_back(warning->text());
                    busyAtWarning = doc.m_communicator != nullptr;
                    progressVisibleAtWarning = progress && progress->isVisible();
                    warning->accept();
                }
            }
            if (!doc.m_communicator && !doc.m_thread.isRunning()) {
                loop.quit();
            }
        });
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        timeout.start(5000);
        poll.start(5);
        doc.m_thread.render(&doc);
        loop.exec();

        REQUIRE(doc.m_communicator == nullptr);
        REQUIRE_FALSE(doc.m_thread.isRunning());
        REQUIRE(progress.isNull());
    };

    SECTION("Standard exception") {
        runAnalysis([]() -> bool { throw std::runtime_error("Module analysis failed"); });
        REQUIRE(messages == std::vector<QString>{"Module analysis failed"});
    }
    SECTION("Unknown exception") {
        runAnalysis([]() -> bool { throw 42; });
        REQUIRE(messages == std::vector<QString>{"An unknown error occurred during analysis."});
    }
    SECTION("Cancellation") {
        runAnalysis([]() -> bool { throw Communicator::CancelledException(); });
        REQUIRE(messages == std::vector<QString>{"Analysis cancelled."});
    }
    SECTION("No module analysis configured") {
        runAnalysis({});
        REQUIRE(messages == std::vector<QString>{"No module analysis was configured."});
    }

    REQUIRE_FALSE(busyAtWarning);
    REQUIRE_FALSE(progressVisibleAtWarning);
    REQUIRE_FALSE(doc.modifiedFlag);
    REQUIRE(doc.GetRedrawFlag(QGraphDoc::VIEW_MAP) == QGraphDoc::REDRAW_GRAPH);
    messages.clear();
    runAnalysis([] { return true; });
    REQUIRE(messages.empty());
    REQUIRE(doc.modifiedFlag);
    REQUIRE(doc.GetRedrawFlag(QGraphDoc::VIEW_MAP) == QGraphDoc::REDRAW_TOTAL);
}
