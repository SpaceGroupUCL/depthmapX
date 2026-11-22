// Copyright (C) 2011-2026, Tasos Varoudis
// This file is part of depthmapX and is licensed under GPL-3.0-or-later.

#include <catch.hpp>

#include <QEventLoop>
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

class TestGraphDoc : public QGraphDoc {
  public:
    TestGraphDoc() : QGraphDoc(QString{}, QString{}) {}

    std::vector<QString> messages;
    bool busyAtWarning = false;
    bool progressOpenAtWarning = false;

  protected:
    void messageFromRenderThread(QString, QString message) override {
        messages.push_back(message);
        busyAtWarning = m_communicator != nullptr || m_thread.isRunning();
        progressOpenAtWarning = m_waitdlg != nullptr;
    }
};
} // namespace

TEST_CASE("The document recovers from a throwing module analysis", "[renderthread]") {
    DefaultSettings settings;
    MainWindow window(QString{}, settings);
    QWidget mapView;
    std::unique_ptr<MetaGraph> graph;
    TestGraphDoc doc;
    graph.reset(doc.m_meta_graph);
    doc.m_mainFrame = &window;
    doc.m_view[QGraphDoc::VIEW_MAP] = &mapView;

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
        QObject::connect(&doc.m_thread, &RenderThread::analysisCompleted, &loop, &QEventLoop::quit,
                         Qt::QueuedConnection);
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        timeout.start(5000);
        doc.m_thread.render(&doc);
        loop.exec();

        REQUIRE(doc.m_communicator == nullptr);
        REQUIRE_FALSE(doc.m_thread.isRunning());
        REQUIRE(progress.isNull());
    };

    SECTION("Standard exception") {
        runAnalysis([]() -> bool { throw std::runtime_error("Module analysis failed"); });
        REQUIRE(doc.messages == std::vector<QString>{"Module analysis failed"});
    }
    SECTION("Unknown exception") {
        runAnalysis([]() -> bool { throw 42; });
        REQUIRE(doc.messages == std::vector<QString>{"An unknown error occurred during analysis."});
    }
    SECTION("Cancellation") {
        runAnalysis([]() -> bool { throw Communicator::CancelledException(); });
        REQUIRE(doc.messages == std::vector<QString>{"Analysis cancelled."});
    }
    SECTION("No module analysis configured") {
        runAnalysis({});
        REQUIRE(doc.messages == std::vector<QString>{"No module analysis was configured."});
    }

    REQUIRE_FALSE(doc.busyAtWarning);
    REQUIRE_FALSE(doc.progressOpenAtWarning);
    REQUIRE_FALSE(doc.modifiedFlag);
    REQUIRE(doc.GetRedrawFlag(QGraphDoc::VIEW_MAP) == QGraphDoc::REDRAW_GRAPH);
    doc.messages.clear();
    runAnalysis([] { return true; });
    REQUIRE(doc.messages.empty());
    REQUIRE(doc.modifiedFlag);
    REQUIRE(doc.GetRedrawFlag(QGraphDoc::VIEW_MAP) == QGraphDoc::REDRAW_TOTAL);
}
