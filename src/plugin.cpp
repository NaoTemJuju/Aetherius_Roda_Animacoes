#include "session.h"
#include "catalog.h"
#include "MeridianUIAPI/ViewDllLoader.h"
#include <thread>
#include <atomic>

SKSEPluginInfo(.Version = "1.0.0.0"_v, .Name = "EmotesSP", .Author = "Alduinak / Aetherius contributors",
  .StructCompatibility = SKSE::StructCompatibility::Independent,
  .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary)

namespace {
using namespace Meridian::UI::View;
WheelSession session;
std::atomic<std::uint64_t> generation{0};
std::atomic<unsigned> tasks{0};
std::string active;
bool registered = false;
bool loaded = false;
auto lastPlay = std::chrono::steady_clock::time_point{};

void Report(const std::string& message) {
  spdlog::info("[EmotesSP] {}", message);
  if (auto* console = RE::ConsoleLog::GetSingleton()) console->Print("[EmotesSP] %s", message.c_str());
  if (session.open && session.api)
    session.api->ExecuteJavaScript(session.view, ("window.EmotesSP.notice(" + nlohmann::json(message).dump() + ");").c_str());
}
// Bounded, one-shot tasks. Delays are paced off-thread, never by a task requeue loop.
void Queue(std::function<void()> fn, unsigned delay = 0) {
  auto* taskInterface = SKSE::GetTaskInterface();
  if (!taskInterface) return;
  if (tasks.fetch_add(1) >= 32) { tasks.fetch_sub(1); return; }
  auto post = [taskInterface, fn = std::move(fn)] {
    taskInterface->AddTask([fn] {
      tasks.fetch_sub(1);
      try { fn(); } catch (const std::exception& e) { spdlog::error("task failed: {}", e.what()); }
    });
  };
  if (!delay) post();
  else std::thread([post, delay] { std::this_thread::sleep_for(std::chrono::milliseconds(delay)); post(); }).detach();
}
bool MenuBlocked() {
  auto* ui = RE::UI::GetSingleton();
  if (!ui) return true;
  for (const char* menu : {"Main Menu", "Loading Menu", "Console", "InventoryMenu", "TweenMenu", "Journal Menu", "MapMenu", "Crafting Menu", "ContainerMenu", "BarterMenu", "RaceSex Menu", "Dialogue Menu", "Book Menu", "GiftMenu", "Lockpicking Menu", "Sleep/Wait Menu"})
    if (ui->IsMenuOpen(menu)) return true;
  return false;
}
bool CanPlay(RE::PlayerCharacter* player, bool allowDrawn = false) {
  return loaded && player && player->Get3D() && !MenuBlocked() && !player->IsDead()
    && !player->IsBleedingOut() && !player->IsOnMount() && !player->IsInCombat()
    && player->GetSitSleepState() == RE::SIT_SLEEP_STATE::kNormal && (allowDrawn || !player->IsWeaponDrawn());
}
bool Event(RE::PlayerCharacter* player, const std::string& id) {
  if (!player || !player->Get3D()) return false;
  const bool accepted = player->NotifyAnimationGraph(RE::BSFixedString(id.c_str()));
  spdlog::info("animation {} accepted={}", id, accepted);
  return accepted;
}
void Stop() {
  ++generation;
  const auto previous = std::exchange(active, {});
  if (previous.empty()) return;
  auto* player = RE::PlayerCharacter::GetSingleton();
  if (!player || player->IsDead()) return;
  if (previous.starts_with("Offset")) Event(player, "OffsetStop");
  else {
    auto base = previous;
    if (base.ends_with("Start")) base.resize(base.size() - 5);
    else if (base.ends_with("Enter")) base.resize(base.size() - 5);
    if (!Event(player, base + "ExitStart") && !Event(player, base + "Exit") && !Event(player, "IdleStop"))
      Event(player, "IdleForceDefaultState");
  }
  Report("Gesto encerrado.");
}
void Start(std::string id, std::uint64_t token, unsigned poll = 0) {
  if (generation != token || !loaded) return;
  auto* player = RE::PlayerCharacter::GetSingleton();
  if (!CanPlay(player, true)) { Report("Use o gesto fora de combate, menus e montaria."); return; }
  if (player->IsWeaponDrawn()) {
    if (poll >= 20) { Report("Não foi possível guardar a arma; pressione R e tente novamente."); return; }
    if (poll == 0) player->DrawWeaponMagicHands(false);
    Queue([id, token, poll] { Start(id, token, poll + 1); }, 180);
    return;
  }
  bool objectLoaded = false;
  player->GetGraphVariableBool("bAnimObjectLoaded", objectLoaded);
  if (objectLoaded) {
    if (poll >= 12) { Report("O objeto do gesto anterior ainda está ativo; mova o personagem e tente novamente."); return; }
    Queue([id, token, poll] { Start(id, token, poll + 1); }, 180);
    return;
  }
  if (auto* camera = RE::PlayerCamera::GetSingleton()) camera->ForceThirdPerson();
  if (Event(player, id)) { active = id; Report("Executado: " + id); }
  else Report("O grafo deste personagem recusou: " + id);
}
void Play(const std::string& id) {
  if (std::find(kEmotes.begin(), kEmotes.end(), id) == kEmotes.end()) return;
  if (!CanPlay(RE::PlayerCharacter::GetSingleton(), true)) { Report("Use o gesto fora de combate, menus e montaria."); return; }
  const auto now = std::chrono::steady_clock::now();
  if (now - lastPlay < 750ms) return;
  lastPlay = now;
  session.Close();
  Stop();
  const auto token = ++generation;
  Queue([id, token] { Start(id, token); }, 180);
}
void Toggle() {
  if (session.open) { session.Close(); return; }
  if (!loaded || MenuBlocked()) return;
  if (!CanPlay(RE::PlayerCharacter::GetSingleton(), true)) { Report("K: saia de combate, montaria e mobiliário."); return; }
  if (session.Show()) Report("Roda aberta — K/Esc fecham; clique em um gesto.");
  else Report("Roda indisponível: ready=" + std::to_string(session.pageReady) + " focus=" + std::to_string(static_cast<unsigned>(session.lastFocus)));
}
void Message(const char* payload) {
  if (!payload) return;
  const auto length = strnlen_s(payload, 1025);
  if (length > 1024) return;
  // Copy CEF-owned data before returning; engine work executes through SKSE.
  const auto message = nlohmann::json::parse(payload, payload + length, nullptr, false);
  if (!message.is_object() || !message.contains("action") || !message["action"].is_string()) return;
  const auto action = message["action"].get<std::string>();
  if (action == "ready") Queue([] { session.pageReady = true; Report("Interface Meridian pronta. Single player 1.0.0 — K."); });
  else if (action == "close") Queue([] { session.Close(); });
  else if (action == "stop") Queue([] { if (session.open) { session.Close(); Stop(); } });
  else if (action == "play" && message.contains("id") && message["id"].is_string()) {
    const auto id = message["id"].get<std::string>();
    if (id.size() <= 80) Queue([id] { if (session.open) Play(id); });
  } else if (action == "ui-error") Queue([] { Report("Erro na página Meridian; consulte MeridianUI.log."); session.Close(); });
}
void DOMReady(ViewHandle handle) {
  Queue([handle] {
    if (handle != session.view || !session.api || !session.api->IsReady(handle)) return;
    session.api->ExecuteJavaScript(handle, "window.EmotesSP?.attach();");
    spdlog::info("DOM ready handle={}", handle);
  });
}
void Create() {
  if (!session.api || session.view) return;
  ViewCreateInfo info{};
  info.ownerName = "emotes-sp"; info.viewName = "GestureWheel";
  info.startUrl = "mod://emotes-sp/index.html";
  info.initiallyVisible = false; info.onDOMReady = DOMReady;
  session.view = session.api->CreateView(&info);
  if (!session.view || !session.api->RegisterListener(session.view, "emotesSPMessage", Message)) {
    Report("Falha ao criar a página local em mod://emotes-sp/index.html.");
    if (session.view) session.api->DestroyView(session.view);
    session.view = 0;
  } else Report("View Meridian criada; aguardando DOM e ponte local.");
}
class Events final : public RE::BSTEventSink<RE::InputEvent*>, public RE::BSTEventSink<RE::MenuOpenCloseEvent>, public RE::BSTEventSink<RE::TESCombatEvent> {
public:
  RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* events, RE::BSTEventSource<RE::InputEvent*>*) override {
    for (auto* event = events ? *events : nullptr; event; event = event->next) {
      const auto* button = event->AsButtonEvent();
      if (!button || button->GetDevice() != RE::INPUT_DEVICE::kKeyboard || !button->IsDown()) continue;
      const auto key = button->GetIDCode();
      if (key == 0x25) Queue(Toggle);
      else if (key == 0x11 || key == 0x1e || key == 0x1f || key == 0x20 || key == 0x39 || key == 0x13) Queue(Stop);
    }
    return RE::BSEventNotifyControl::kContinue;
  }
  RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
    if (event && event->opening && event->menuName != "FocusMenu" && event->menuName != "HUD Menu")
      Queue([] { if (MenuBlocked()) session.Close(); });
    return RE::BSEventNotifyControl::kContinue;
  }
  RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* event, RE::BSTEventSource<RE::TESCombatEvent>*) override {
    if (event && event->actor && event->actor->GetFormID() == 0x14)
      Queue([] { if (auto* p = RE::PlayerCharacter::GetSingleton(); p && p->IsInCombat()) { session.Close(); Stop(); } });
    return RE::BSEventNotifyControl::kContinue;
  }
};
Events events;
void Lifecycle(SKSE::MessagingInterface::Message* message) {
  if (!message) return;
  spdlog::info("SKSE lifecycle {}", message->type);
  if (message->type == SKSE::MessagingInterface::kInputLoaded) {
    Meridian::UI::Settings settings{};
    session.api = Query(&settings, "emotes-sp");
    if (!session.api) spdlog::error("Meridian.View/1 unavailable; install Meridian UI.");
  } else if (message->type == SKSE::MessagingInterface::kDataLoaded && !registered) {
    registered = true;
    if (auto* input = RE::BSInputDeviceManager::GetSingleton()) input->AddEventSink(&events);
    if (auto* ui = RE::UI::GetSingleton()) ui->AddEventSink<RE::MenuOpenCloseEvent>(&events);
    if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) holder->AddEventSink<RE::TESCombatEvent>(&events);
    Queue(Create);
  } else if (message->type == SKSE::MessagingInterface::kPreLoadGame) {
    loaded = false; ++generation; active.clear(); Queue([] { session.Close(); });
  } else if (message->type == SKSE::MessagingInterface::kPostLoadGame || message->type == SKSE::MessagingInterface::kNewGame) {
    loaded = message->type == SKSE::MessagingInterface::kNewGame || reinterpret_cast<std::uintptr_t>(message->data) != 0;
    Queue([] { Report("Single player 1.0.0 carregado. Tecla K abre os gestos."); });
  }
}
} // namespace

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
  if (!skse || skse->IsEditor()) return false;
  auto path = SKSE::log::log_directory();
  if (!path) return false;
  *path /= "EmotesSP.log";
  auto logger = std::make_shared<spdlog::logger>("EmotesSP", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
  logger->set_level(spdlog::level::info); logger->flush_on(spdlog::level::info);
  spdlog::set_default_logger(logger);
  SKSE::Init(skse, false);
  spdlog::info("EmotesSP 1.0.0 native SP fork. No Skyrim Platform, no network client.");
  return SKSE::GetMessagingInterface() && SKSE::GetMessagingInterface()->RegisterListener(Lifecycle);
}
