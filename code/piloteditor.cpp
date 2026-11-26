#include "piloteditor.h"

#include <cstdio>
#include "afont.h"
#include "warrior.h"
#include "../resource.h"

namespace
{
constexpr long pilotPanelWidth = 568;
constexpr long pilotPanelHeight = 414;
constexpr int pilotSkillRows = (NUM_SPECIALTY_SKILLS + 1) / 2;
constexpr float maxEditedPilotSkill = 80.0f;

GUI_RECT pilotStatRect(const GUI_RECT& panel, int row)
{
    const long x = panel.left + 12 + 184 * row;
    return {x, panel.top + 59, x + 176, panel.top + 87};
}

GUI_RECT pilotStatButton(const GUI_RECT& rect, int direction)
{
    if (direction < 0)
        return {rect.left, rect.top, rect.left + 25, rect.bottom};
    return {rect.right - 25, rect.top, rect.right, rect.bottom};
}

GUI_RECT pilotSkillRect(const GUI_RECT& panel, int skill)
{
    const long x = panel.left + 12 + 276 * (skill / pilotSkillRows);
    const long y = panel.top + 102 + 23 * (skill % pilotSkillRows);
    return {x, y, x + 268, y + 20};
}

GUI_RECT pilotBackRect(const GUI_RECT& panel)
{
    return {panel.right - 123, panel.bottom - 29, panel.right - 12, panel.bottom - 9};
}

float clampPilotValue(float value, float minimum, float maximum)
{
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

bool stepPilotRank(PilotEditor::Values& pilot, int direction)
{
    const int rank = pilot.rank + direction;
    if (rank < WARRIOR_RANK_GREEN || rank > WARRIOR_RANK_ACE)
        return false;

    const float averages[NUM_WARRIOR_RANKS] = {45.0f, 55.0f, 65.0f, 75.0f, 80.0f};
    const float total = 2.0f * averages[rank];
    const float balance = pilot.skills[0] - pilot.skills[1];
    const float minimum = total - maxEditedPilotSkill > 1.0f ? total - maxEditedPilotSkill : 1.0f;
    const float maximum = total - 1.0f < maxEditedPilotSkill ? total - 1.0f : maxEditedPilotSkill;
    pilot.skills[0] = clampPilotValue((total + balance) / 2.0f, minimum, maximum);
    pilot.skills[1] = total - pilot.skills[0];
    return true;
}

bool contains(const GUI_RECT& rect, long x, long y)
{
    return x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom;
}
}

GUI_RECT PilotEditor::panelRect(long width, long height)
{
    const long left = (width - pilotPanelWidth) / 2;
    const long top = (height - pilotPanelHeight) / 2;
    return {left, top, left + pilotPanelWidth, top + pilotPanelHeight};
}

PilotEditor::Result PilotEditor::click(Values& pilot, const GUI_RECT& panel, long x, long y)
{
    if (contains(pilotBackRect(panel), x, y))
        return Result::Close;

    for (int field = 0; field < 3; ++field)
    {
        const GUI_RECT rect = pilotStatRect(panel, field);
        for (int direction = -1; direction <= 1; direction += 2)
        {
            if (!contains(pilotStatButton(rect, direction), x, y))
                continue;
            if (field == 0)
                return stepPilotRank(pilot, direction) ? Result::Stats : Result::None;

            float& skill = pilot.skills[field - 1];
            const float value = clampPilotValue(skill + direction, 1.0f, maxEditedPilotSkill);
            if (skill == value)
                return Result::None;
            skill = value;
            return Result::Stats;
        }
    }

    for (int skill = 0; skill < NUM_SPECIALTY_SKILLS; ++skill)
    {
        if (!contains(pilotSkillRect(panel, skill), x, y))
            continue;
        pilot.specialtySkills[skill] = !pilot.specialtySkills[skill];
        return Result::Specialty;
    }
    return Result::None;
}

void PilotEditor::render(const Values& pilot, const GUI_RECT& panel, long mouseX, long mouseY,
                         aFont& font, aFont& headerFont)
{
    gos_SetRenderState(gos_State_AlphaMode, gos_Alpha_AlphaInvAlpha);
    gos_SetRenderState(gos_State_ZWrite, 0);
    gos_SetRenderState(gos_State_ZCompare, 0);
    drawRect(panel, 0xf018222a);
    drawRect({panel.left, panel.top, panel.right, panel.top + 2}, 0xff6ba6c7);

    char title[96];
    snprintf(title, sizeof(title), "PILOT EDITOR  /  %s", pilot.name);
    headerFont.render(title, panel.left + 12, panel.top + 13, pilotPanelWidth - 24, 28, 0xff8bc6e4, false, 0);

    char rank[32], gunnery[48], piloting[48], rankLabel[48];
    const int rankId = pilot.rank == WARRIOR_RANK_ACE ? IDS_ACE : IDS_GREEN + pilot.rank;
    cLoadString(rankId, rank, sizeof(rank));
    snprintf(rankLabel, sizeof(rankLabel), "Rank: %s", rank);
    snprintf(gunnery, sizeof(gunnery), "Gunnery: %ld", static_cast<long>(pilot.skills[0]));
    snprintf(piloting, sizeof(piloting), "Piloting: %ld", static_cast<long>(pilot.skills[1]));
    const char* stats[] = {rankLabel, gunnery, piloting};
    for (int field = 0; field < 3; ++field)
    {
        const GUI_RECT rect = pilotStatRect(panel, field);
        drawRect(rect, 0xff263b47);
        font.render(stats[field], rect.left + 31, rect.top + 7, 115, 18, 0xffedf4f7, false, 0);
        for (int direction = -1; direction <= 1; direction += 2)
        {
            const GUI_RECT button = pilotStatButton(rect, direction);
            drawRect(button, contains(button, mouseX, mouseY) ? 0xff52788b : 0xff385c70);
            font.render(direction < 0 ? "-" : "+", button.left + 9, button.top + 7, 15, 18, 0xffedf4f7, false, 0);
        }
    }

    for (int skill = 0; skill < NUM_SPECIALTY_SKILLS; ++skill)
    {
        const GUI_RECT row = pilotSkillRect(panel, skill);
        const bool enabled = pilot.specialtySkills[skill];
        const bool hover = contains(row, mouseX, mouseY);
        drawRect(row, hover ? 0xff385c70 : 0xff263b47);
        char label[128];
        cLoadString(IDS_SPECIALTY + skill, label, sizeof(label));
        font.render(label, row.left + 7, row.top + 3, 212, 17, 0xffedf4f7, false, 0);
        font.render(enabled ? "ON" : "OFF", row.right - 7, row.top + 3, 0, 17,
                          enabled ? 0xffa4df9f : 0xffadc4cf, false, 1);
    }

    const GUI_RECT back = pilotBackRect(panel);
    drawRect(back, contains(back, mouseX, mouseY) ? 0xff385c70 : 0xff263b47);
    font.render("Back / Esc", back.left + 8, back.top + 3, back.right - back.left - 14, 17, 0xffedf4f7, false, 0);
}
