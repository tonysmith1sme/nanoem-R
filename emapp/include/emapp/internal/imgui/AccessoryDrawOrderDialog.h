/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#pragma once
#ifndef NANOEM_EMAPP_INTERNAL_IMGUI_ACCESSORYDRAWORDERDIALOG_H_
#define NANOEM_EMAPP_INTERNAL_IMGUI_ACCESSORYDRAWORDERDIALOG_H_

#include "emapp/internal/imgui/BaseNonModalDialogWindow.h"

namespace nanoem {
class Accessory;
class IDrawable;
class Project;

namespace internal {
namespace imgui {

struct AccessoryDrawOrderState {
    Project::DrawableList m_currentDrawableList;
    IDrawable *m_checkedDrawable;

    AccessoryDrawOrderState(const Project::DrawableList &value)
        : m_currentDrawableList(value)
        , m_checkedDrawable(nullptr)
    {
    }
    void
    setOrderAt(int index)
    {
        if (m_checkedDrawable) {
            for (auto it = m_currentDrawableList.begin(), end = m_currentDrawableList.end(); it != end; ++it) {
                if (*it == m_checkedDrawable && it + index >= m_currentDrawableList.begin() &&
                    it + index < m_currentDrawableList.end()) {
                    m_currentDrawableList.erase(it);
                    m_currentDrawableList.insert(it + index, m_checkedDrawable);
                    break;
                }
            }
        }
    }
    bool
    isOrderBegin() const
    {
        return m_currentDrawableList.size() <= 1 ||
            (m_checkedDrawable && m_checkedDrawable == *m_currentDrawableList.begin());
    }
    bool
    isOrderEnd() const
    {
        return m_currentDrawableList.size() <= 1 ||
            (m_checkedDrawable && m_checkedDrawable == *(m_currentDrawableList.end() - 1));
    }
};

struct AccessoryDrawOrderDialog : BaseNonModalDialogWindow {
    static const char *const kIdentifier;

    AccessoryDrawOrderDialog(Project *project, BaseApplicationService *applicationPtr);

    bool draw(Project *project);

    Project::DrawableList m_lastDrawableOrderList;
    Project::DrawableList m_lastAccessoryOrderList;
    AccessoryDrawOrderState m_orderState;
    Accessory *m_renamingAccessory;
    char m_renameBuffer[256];
};

} /* namespace imgui */
} /* namespace internal */
} /* namespace nanoem */

#endif /* NANOEM_EMAPP_INTERNAL_IMGUI_ACCESSORYDRAWORDERDIALOG_H_ */
