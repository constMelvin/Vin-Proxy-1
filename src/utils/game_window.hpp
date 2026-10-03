#pragma once

namespace utils::game_window {

// Presses Esc in the Growtopia window. Growtopia (Proton SDK) maps Esc to its "Back" button:
// Back on the world select menu, Cancel on the connecting screen, close on dialogs.
// Returns false when no Growtopia window was found.
bool press_back();

}
