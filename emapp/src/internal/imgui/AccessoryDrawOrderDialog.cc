/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "emapp/internal/imgui/AccessoryDrawOrderDialog.h"

#include "emapp/Accessory.h"
#include "emapp/Project.h"
#include "emapp/StringUtils.h"
#include "emapp/internal/ImGuiWindow.h"

namespace nanoem {
namespace internal {
namespace imgui {

const char *const AccessoryDrawOrderDialog::kIdentifier = "dialog.accessory.order.draw";

static Project::DrawableList
extractAccessoryOrderList(const Project::DrawableList &fullList, const Project::DrawableList &fallbackList)
{
    Project::DrawableList result;
    for (auto it = fullList.begin(), end = fullList.end(); it != end; ++it) {
        if (dynamic_cast<Accessory *>(*it)) {
            result.push_back(*it);
        }
    }
    if (result.empty() && !fallbackList.empty()) {
        for (auto it = fallbackList.begin(), end = fallbackList.end(); it != end; ++it) {
            if (dynamic_cast<Accessory *>(*it)) {
                result.push_back(*it);
            }
        }
    }
    return result;
}

static void
applyAccessoryOrderToDrawableList(Project *project, const Project::DrawableList &accessoryOrder)
{
    Project::DrawableList fullList;
    Project::DrawableList accessoriesCopy(accessoryOrder);
    const Project::DrawableList *currentList = project->drawableOrderList();
    for (auto it = currentList->begin(), end = currentList->end(); it != end; ++it) {
        if (dynamic_cast<Accessory *>(*it)) {
            bool found = false;
            for (auto it2 = accessoriesCopy.begin(), end2 = accessoriesCopy.end(); it2 != end2; ++it2) {
                if (*it2 == *it) {
                    accessoriesCopy.erase(it2);
                    found = true;
                    break;
                }
            }
            if (!found) {
                fullList.push_back(*it);
            }
        }
        else {
            fullList.push_back(*it);
        }
    }
    for (auto it = accessoriesCopy.begin(), end = accessoriesCopy.end(); it != end; ++it) {
        fullList.push_back(*it);
    }
    project->setDrawableOrderList(fullList);
}

AccessoryDrawOrderDialog::AccessoryDrawOrderDialog(Project *project, BaseApplicationService *applicationPtr)
    : BaseNonModalDialogWindow(applicationPtr)
    , m_lastDrawableOrderList(*project->drawableOrderList())
    , m_lastAccessoryOrderList(extractAccessoryOrderList(*project->drawableOrderList(), m_lastDrawableOrderList))
    , m_orderState(m_lastAccessoryOrderList)
    , m_renamingAccessory(nullptr)
{
    m_renameBuffer[0] = 0;
}

bool
AccessoryDrawOrderDialog::draw(Project *project)
{
    bool visible = true;
    const nanoem_f32_t height = ImGui::GetFrameHeightWithSpacing() * 14;
    if (open(tr("nanoem.gui.window.project.order.accessorydrawable.title"), kIdentifier, &visible, height)) {
        bool changed = false;
        if (ImGuiWindow::handleButton("Up", ImGui::GetContentRegionAvail().x * 0.5f,
                !m_orderState.isOrderBegin())) {
            m_orderState.setOrderAt(-1);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGuiWindow::handleButton("Down", ImGui::GetContentRegionAvail().x,
                !m_orderState.isOrderEnd())) {
            m_orderState.setOrderAt(1);
            changed = true;
        }
        ImGui::BeginChild("##accessories", ImVec2(0, height - ImGui::GetFrameHeightWithSpacing() * 3.5f), true);
        for (auto it = m_orderState.m_currentDrawableList.begin(),
                  end = m_orderState.m_currentDrawableList.end();
             it != end; ++it) {
            IDrawable *drawable = *it;
            Accessory *accessory = dynamic_cast<Accessory *>(drawable);
            if (!accessory) {
                continue;
            }
            const bool isRenaming = (m_renamingAccessory == accessory);
            const bool isSelected = (drawable == m_orderState.m_checkedDrawable);
            if (isRenaming) {
                if (m_renameBuffer[0] == 0) {
                    StringUtils::copyString(m_renameBuffer, accessory->nameConstString(), sizeof(m_renameBuffer));
                }
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
                if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer),
                        ImGuiInputTextFlags_EnterReturnsTrue)) {
                    if (m_renameBuffer[0] != 0) {
                        accessory->setName(m_renameBuffer);
                    }
                    m_renamingAccessory = nullptr;
                    m_renameBuffer[0] = 0;
                }
                ImGui::SameLine();
                if (ImGui::Button("OK") || (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))) {
                    if (m_renameBuffer[0] != 0) {
                        accessory->setName(m_renameBuffer);
                    }
                    m_renamingAccessory = nullptr;
                    m_renameBuffer[0] = 0;
                }
            }
            else {
                if (ImGui::Selectable(accessory->nameConstString(), isSelected)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        m_renamingAccessory = accessory;
                        m_renameBuffer[0] = 0;
                    }
                    else {
                        m_orderState.m_checkedDrawable = drawable;
                    }
                }
            }
        }
        ImGui::EndChild();
        if (changed) {
            applyAccessoryOrderToDrawableList(project, m_orderState.m_currentDrawableList);
        }
        switch (layoutCommonButtons(&visible)) {
        case kResponseTypeOK: {
            applyAccessoryOrderToDrawableList(project, m_orderState.m_currentDrawableList);
            break;
        }
        case kResponseTypeCancel: {
            project->setDrawableOrderList(m_lastDrawableOrderList);
            break;
        }
        default:
            break;
        }
    }
    else if (!visible) {
        project->setDrawableOrderList(m_lastDrawableOrderList);
    }
    close();
    return visible;
}

} /* namespace imgui */
} /* namespace internal */
} /* namespace nanoem */
