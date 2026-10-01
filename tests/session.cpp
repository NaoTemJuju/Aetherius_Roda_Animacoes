#include "session.h"
#include <cstdio>
#include <stdexcept>
#include <string>
using namespace Meridian::UI::View;
struct Fake final : IViewAPI {
  bool ready = false, visible = false, focused = false, otherFocus = false;
  unsigned focusCalls = 0;
  FocusResult result = FocusResult::Granted;
  ViewHandle CreateView(const ViewCreateInfo*) override { return 1; }
  void DestroyView(ViewHandle) override {}
  bool IsValid(ViewHandle h) const override { return h == 1; }
  bool IsReady(ViewHandle) const override { return ready; }
  bool RegisterListener(ViewHandle,const char*,ListenerCallback) override { return true; }
  bool ExecuteJavaScript(ViewHandle,const char*) override { return ready; }
  bool Show(ViewHandle) override { visible = true; return true; }
  bool Hide(ViewHandle) override { visible = false; return true; }
  FocusResult TryFocus(ViewHandle,FocusMode) override { ++focusCalls; return result; }
  void Unfocus(ViewHandle) override { focused = false; }
  bool HasFocus(ViewHandle) const override { return focused; }
  bool HasAnyFocus() const override { return otherFocus || focused; }
};
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
int main() {
  try {
    Fake api; WheelSession s; s.api = &api; s.view = 1;
    Check(!s.Show() && !api.visible && !api.focusCalls, "404 cannot focus");
    api.ready = true;
    Check(!s.Show(), "DOM without JS bridge cannot focus");
    s.pageReady = true;
    Check(s.Show() && s.open && api.visible, "ready wheel opens");
    Check(!api.focused && s.open, "asynchronous FocusMenu cannot close wheel");
    Check(s.Show() && api.focusCalls == 1, "repeated show is idempotent");
    api.focused = true; s.Close();
    Check(!s.open && !api.visible && !api.focused, "close releases visibility and focus");
    api.otherFocus = true;
    Check(!s.Show() && api.focusCalls == 1, "other consumer retains focus");
    api.otherFocus = false; api.result = FocusResult::Busy;
    Check(!s.Show() && !api.visible && !s.open, "focus rejection hides wheel");
    s.Close();
    std::puts("PASS 8 native Meridian session checks (fake provider; Skyrim not simulated)");
  } catch(const std::exception& e) { std::fprintf(stderr,"FAIL %s\n",e.what()); return 1; }
}
