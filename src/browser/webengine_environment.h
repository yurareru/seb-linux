#pragma once

#include "../settings/Browser/BrowserSettings.h"

namespace seb::browser {

void applyWebEngineEnvironment(const seb::settings::browser::BrowserSettings &settings);

// Must run before Qt WebEngine is initialized (Chromium reads its flags only once).
void applyMediaCaptureEnvironment();

}  // namespace seb::browser
