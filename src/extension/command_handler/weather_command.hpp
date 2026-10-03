#pragma once
#include "command_base.hpp"
#include "../../core/core.hpp"
#include <string>
#include <vector>

namespace player { class Player; }

namespace command {

// WeatherCommand - Custom Weather Machine (LuckyProxy /weather)
class WeatherCommand : public CommandBase {
public:
    WeatherCommand();
    void execute(client::Client* client, const std::vector<std::string>& args) override;
    std::unique_ptr<CommandBase> clone() const override;

    static void set_core(core::Core* core);
    // Handles the "weather_page" dialog_return (raw = full dialog_return text)
    static void handle_dialog_response(player::Player* player, const std::string& button_clicked, const std::string& raw);
    // Re-applies the custom weather on world join when "keep weather" is enabled
    static void on_local_spawn(player::Player* player);

private:
    static void show_dialog(player::Player* player);
    static void send_weather_packet(player::Player* player, int weather_id);

    static core::Core* s_core;
    static bool s_keep_weather;
    static int s_current_weather;
};

}
