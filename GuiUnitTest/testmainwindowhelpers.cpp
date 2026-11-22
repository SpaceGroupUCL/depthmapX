// Copyright (C) 2011-2026, Tasos Varoudis
// This file is part of depthmapX and is licensed under GPL-3.0-or-later.

#include "catch.hpp"
#include "mainwindowhelpers.h"

TEST_CASE("A module can insert a submenu before an existing action", "[module-menu]") {
    QMenu tools;
    auto *visibility = tools.addMenu("Visibility");
    auto *options = tools.addAction("Options");

    auto *module = MainWindowHelpers::getOrAddMenu(&tools, "Module", options);
    REQUIRE(tools.actions() == QList<QAction *>{visibility->menuAction(), module->menuAction(), options});
    REQUIRE(MainWindowHelpers::getOrAddMenu(&tools, "Module", visibility->menuAction()) == module);
    REQUIRE(tools.actions().at(1) == module->menuAction());

    auto *last = MainWindowHelpers::getOrAddMenu(&tools, "Last");
    REQUIRE(tools.actions().last() == last->menuAction());
}
