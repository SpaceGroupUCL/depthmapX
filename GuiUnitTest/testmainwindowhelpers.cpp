// Copyright (C) 2011-2026, Tasos Varoudis
// This file is part of depthmapX and is licensed under GPL-3.0-or-later.

#include "catch.hpp"
#include "mainwindowhelpers.h"

TEST_CASE("A module can insert a submenu before an existing action", "[module-menu]") {
    QMenu tools;
    auto *visibility = tools.addMenu("Visibility");
    auto *options = tools.addAction("Options");

    auto *module = MainWindowHelpers::getOrAddMenu(&tools, "Module", options);
    const auto actions = tools.actions();
    REQUIRE(actions.size() == 3);
    CHECK(actions.at(0) == visibility->menuAction());
    CHECK(actions.at(1) == module->menuAction());
    CHECK(actions.at(2) == options);
    REQUIRE(MainWindowHelpers::getOrAddMenu(&tools, "Module", visibility->menuAction()) == module);
    REQUIRE(tools.actions().at(1) == module->menuAction());

    auto *last = MainWindowHelpers::getOrAddMenu(&tools, "Last");
    REQUIRE(tools.actions().last() == last->menuAction());
}
