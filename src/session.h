#pragma once
#include "MeridianUIAPI/ViewAPI.h"

// No Skyrim or multiplayer types: the native consumer owns one Meridian view.
// FocusMenu may open after TryFocus returns. Never treat that delay as a close.
class WheelSession {
public:
  Meridian::UI::View::IViewAPI* api = nullptr;
  Meridian::UI::View::ViewHandle view = 0;
  bool pageReady = false;
  bool open = false;
  Meridian::UI::View::FocusResult lastFocus = Meridian::UI::View::FocusResult::NotReady;
  bool Show() {
    using namespace Meridian::UI::View;
    if (!api || !view || !pageReady || !api->IsValid(view) || !api->IsReady(view)) return false;
    if (open) return true;
    if (api->HasAnyFocus() && !api->HasFocus(view)) { lastFocus = FocusResult::Busy; return false; }
    if (!api->ExecuteJavaScript(view, "window.EmotesSP.setOpen(true);") || !api->Show(view)) return false;
    lastFocus = api->TryFocus(view, FocusMode::Unpaused);
    if (lastFocus != FocusResult::Granted && lastFocus != FocusResult::AlreadyFocused) {
      api->ExecuteJavaScript(view, "window.EmotesSP.setOpen(false);");
      api->Hide(view);
      return false;
    }
    open = true;
    return true;
  }
  void Close() {
    open = false;
    if (!api || !view || !api->IsValid(view)) return;
    api->ExecuteJavaScript(view, "window.EmotesSP.setOpen(false);");
    api->Unfocus(view);
    api->Hide(view);
  }
};
