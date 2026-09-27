#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"

namespace command {

// =============================================
// ShowXY Command - Toggle X,Y Position Display
// =============================================
class ShowXYCommand : public CommandBase {
public:
    ShowXYCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// UidCommand - Show Current Player UID & NetID
// =============================================
class UidCommand : public CommandBase {
public:
    UidCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

// =============================================
// ScanCommand - Toggle World Item Scan / Extract Mode
// =============================================
class ScanCommand : public CommandBase {
public:
    ScanCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// TrackCommand - Toggle Drop Tracker
// =============================================
class TrackCommand : public CommandBase {
public:
    TrackCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// GhostCommand - Toggle Moderator Ghost Mode
// =============================================
class GhostCommand : public CommandBase {
public:
    GhostCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// AutoMsgCommand - Toggle Auto Message Broadcast
// =============================================
class AutoMsgCommand : public CommandBase {
public:
    AutoMsgCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// AutoPullCommand - Toggle Auto Pull Players
// =============================================
class AutoPullCommand : public CommandBase {
public:
    AutoPullCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// FastRecycleCommand - Toggle Fast Recycle
// =============================================
class FastRecycleCommand : public CommandBase {
public:
    FastRecycleCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// WrenchMsgCommand - Toggle Wrench Chat Messages
// =============================================
class WrenchMsgCommand : public CommandBase {
public:
    WrenchMsgCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// WrenchSpamCommand - Toggle Wrench Spam Mode
// =============================================
class WrenchSpamCommand : public CommandBase {
public:
    WrenchSpamCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// BlinkCommand - Toggle Rainbow Blink Mode
// =============================================
class BlinkCommand : public CommandBase {
public:
    BlinkCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// FastVendToggleCommand - Toggle Fast Vend Mode
// =============================================
class FastVendToggleCommand : public CommandBase {
public:
    FastVendToggleCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// SpeedCommand - Set Movement Speed Multiplier
// =============================================
class SpeedCommand : public CommandBase {
public:
    SpeedCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static int get_speed();
private:
    static core::Core* s_core;
    static int s_speed;
};

// =============================================
// SetTaxCommand - Set Tax Percentage
// =============================================
class SetTaxCommand : public CommandBase {
public:
    SetTaxCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static int get_tax();
private:
    static core::Core* s_core;
    static int s_tax;
};

// =============================================
// GameBetCommand - Calculate Game Bet Tax
// =============================================
class GameBetCommand : public CommandBase {
public:
    GameBetCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static int get_last_drop_amount();
private:
    static core::Core* s_core;
    static int s_last_drop_amount;
};

// =============================================
// GDropCommand - Drop Prize With Tax Deduction
// =============================================
class GDropCommand : public CommandBase {
public:
    GDropCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

// =============================================
// CollectCommand - Collect Floating Items (10 tile radius)
// =============================================
class CollectCommand : public CommandBase {
public:
    CollectCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

// =============================================
// CountryListCommand - Show All Country Flag IDs
// =============================================
class CountryListCommand : public CommandBase {
public:
    CountryListCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

// =============================================
// WorldOptionsCommand - World Management Dialog
// =============================================
class WorldOptionsCommand : public CommandBase {
public:
    WorldOptionsCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

// =============================================
// AutoFishCommand - Auto Fish Bot Settings
// =============================================
class AutoFishCommand : public CommandBase {
public:
    AutoFishCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// AutoFarmCommand - Auto Farm Bot Settings
// =============================================
class AutoFarmCommand : public CommandBase {
public:
    AutoFarmCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
    static bool is_enabled();
private:
    static core::Core* s_core;
    static bool s_enabled;
};

// =============================================
// HotkeysCommand - Hotkey Shortcuts Config
// =============================================
class HotkeysCommand : public CommandBase {
public:
    HotkeysCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

// =============================================
// OptionsPageCommand - All Features Options Page
// =============================================
class OptionsPageCommand : public CommandBase {
public:
    OptionsPageCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

// =============================================
// DropBGLAliasCommand - Drop Specified Amount of BGLs
// =============================================
class DropBGLAliasCommand : public CommandBase {
public:
    DropBGLAliasCommand();
    std::unique_ptr<CommandBase> clone() const override;
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    static void set_core(core::Core* core);
private:
    static core::Core* s_core;
};

} // namespace command
