#ifndef PILOTEDITOR_H
#define PILOTEDITOR_H

#include "utilities.h"

class aFont;

namespace PilotEditor
{
struct Values
{
    const char* name;
    int rank;
    float skills[2]; // Gunnery, then piloting.
    bool* specialtySkills;
};

enum class Result { None, Close, Stats, Specialty };

GUI_RECT panelRect(long width, long height);
Result click(Values& pilot, const GUI_RECT& panel, long x, long y);
void render(const Values& pilot, const GUI_RECT& panel, long mouseX, long mouseY,
            aFont& font, aFont& headerFont);
}

#endif
