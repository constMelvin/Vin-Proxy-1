#include "dialog.hpp"
#include "byte_stream.hpp"
#include "../player/player.hpp"
#include "../packet/packet_types.hpp"
#include "../packet/packet_variant.hpp"
#include <algorithm>

namespace utils {

Dialog& Dialog::line(const std::string& text) {
    data_ += text;
    data_ += '\n';
    return *this;
}

const char* Dialog::size_name(Size size) {
    return size == Size::Big ? "big" : "small";
}

const char* Dialog::align_name(Align align) {
    switch (align) {
        case Align::Center: return "center";
        case Align::Right: return "right";
        default: return "left";
    }
}

std::string Dialog::sanitize(std::string text) {
    text.erase(std::remove_if(text.begin(), text.end(), [](char c) { return c == '|' || c == '\n' || c == '\r'; }),
               text.end());
    return text;
}

// ---------------------------------------------------------------------------
// Dialog settings
// ---------------------------------------------------------------------------
Dialog& Dialog::set_default_color(const std::string& color) { return line("set_default_color|" + color); }
Dialog& Dialog::set_bg_color(const std::string& rgba) { return line("set_bg_color|" + rgba + "|"); }
Dialog& Dialog::set_border_color(const std::string& rgba) { return line("set_border_color|" + rgba + "|"); }
Dialog& Dialog::set_custom_spacing(const std::string& spacing) { return line("set_custom_spacing|" + spacing + "|"); }
Dialog& Dialog::text_scaling_string(const std::string& text) { return line("text_scaling_string|" + text + "|"); }
Dialog& Dialog::popup_name(const std::string& name) { return line("add_popup_name|" + name + "|"); }

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------
Dialog& Dialog::label_with_icon(const std::string& text, int icon, Size size) {
    return line(std::string("add_label_with_icon|") + size_name(size) + "|" + text + "|left|" + std::to_string(icon) + "|");
}

Dialog& Dialog::label_with_ele_icon(const std::string& text, int icon, int element, Size size) {
    return line(std::string("add_label_with_ele_icon|") + size_name(size) + "|" + text + "|left|" +
                std::to_string(icon) + "|" + std::to_string(element) + "|");
}

Dialog& Dialog::label(const std::string& text, Size size) {
    return line(std::string("add_label|") + size_name(size) + "|" + text + "|left|0|");
}

Dialog& Dialog::textbox(const std::string& text, Align align) {
    return line("add_textbox|" + text + "|" + align_name(align) + "|");
}

Dialog& Dialog::smalltext(const std::string& text) { return line("add_smalltext|" + text + "|"); }
Dialog& Dialog::smalltext_forced(const std::string& text) { return line("add_smalltext_forced|" + text + "|left|"); }

Dialog& Dialog::smalltext_forced_alpha(const std::string& text, const std::string& alpha) {
    return line("add_smalltext_forced_alpha|" + text + "|" + alpha + "|left|");
}

Dialog& Dialog::custom_textbox(const std::string& text, const std::string& props) {
    return line("add_custom_textbox|" + text + "|" + props + "|");
}

Dialog& Dialog::description(const std::string& text) {
    custom_margin(0, -32);
    custom_textbox("`o" + text, std::string("size:tiny;color:") + kGreyText);
    return custom_margin(0, 10);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
Dialog& Dialog::spacer(Size size) { return line(std::string("add_spacer|") + size_name(size) + "|"); }

Dialog& Dialog::custom_margin(int x, int y) {
    return line("add_custom_margin|x:" + std::to_string(x) + ";y:" + std::to_string(y) + "|");
}

Dialog& Dialog::custom_break() { return line("add_custom_break|"); }

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------
Dialog& Dialog::button(const std::string& id, const std::string& label, const std::string& flags) {
    return line("add_button|" + id + "|" + label + "|" + flags + "|0|0|");
}

Dialog& Dialog::label_with_icon_button(const std::string& text, int icon, const std::string& button_id) {
    return line("add_label_with_icon_button||" + text + "|left|" + std::to_string(icon) + "|" + button_id + "|");
}

Dialog& Dialog::button_with_icon(const std::string& id, const std::string& text, int item_id,
                                 const std::string& frame, const std::string& count) {
    return line("add_button_with_icon|" + id + "|" + text + "|" + frame + "|" + std::to_string(item_id) + "|" +
                (count.empty() ? "" : count + "|"));
}

Dialog& Dialog::end_list() { return line("add_button_with_icon||END_LIST|noflags|0||"); }

Dialog& Dialog::custom_button(const std::string& id, const std::string& props) {
    return line("add_custom_button|" + id + "|" + props + "|");
}

Dialog& Dialog::image_button(const std::string& id, const std::string& image_path,
                             const std::string& layout, const std::string& link) {
    return line("add_image_button|" + id + "|" + image_path + "|" + layout + "|" + link + "||");
}

Dialog& Dialog::url_button(const std::string& id, const std::string& label, const std::string& url) {
    return line("add_url_button|" + id + "|" + label + "|NOFLAGS|" + url + "|");
}

Dialog& Dialog::world_button(const std::string& label, const std::string& world) {
    return line("add_url_button||" + label + "|NOFLAGS|OPENWORLD|" + world + "|");
}

Dialog& Dialog::achieve(const std::string& title, const std::string& description, int icon) {
    return line("add_achieve|" + title + "|" + description + "|" + std::to_string(icon) + "|");
}

// ---------------------------------------------------------------------------
// Inputs
// ---------------------------------------------------------------------------
Dialog& Dialog::checkbox(const std::string& id, const std::string& label, bool checked) {
    return line("add_checkbox|" + id + "|" + label + "|" + (checked ? "1" : "0") + "|");
}

Dialog& Dialog::text_input(const std::string& id, const std::string& label, const std::string& value, int max_length) {
    return line("add_text_input|" + id + "|" + label + "|" + value + "|" + std::to_string(max_length) + "|");
}

Dialog& Dialog::text_input_with_info(const std::string& id, const std::string& label, const std::string& value) {
    return line("add_text_input_with_info|" + id + "|" + label + "|" + value + "|");
}

Dialog& Dialog::text_box_input(const std::string& id, const std::string& label, const std::string& value,
                               int max_length, int lines) {
    return line("add_text_box_input|" + id + "|" + label + "|" + value + "|" + std::to_string(max_length) + "|" +
                std::to_string(lines) + "|");
}

Dialog& Dialog::item_picker(const std::string& id, const std::string& label, const std::string& header) {
    return line("add_item_picker|" + id + "|" + label + "|" + header + "|");
}

Dialog& Dialog::player_picker(const std::string& id, const std::string& label) {
    return line("add_player_picker|" + id + "|" + label + "|");
}

Dialog& Dialog::embed_data(const std::string& key, const std::string& value) {
    return line("embed_data|" + key + "|" + value);
}

// ---------------------------------------------------------------------------
// Special
// ---------------------------------------------------------------------------
Dialog& Dialog::player_info(const std::string& name, int level, int xp, int xp_needed) {
    return line("add_player_info|" + name + "|" + std::to_string(level) + "|" + std::to_string(xp) + "|" +
                std::to_string(xp_needed) + "|");
}

Dialog& Dialog::label_with_icon_button_list(const std::string& format, const std::string& button_prefix,
                                            const std::string& fields, const std::string& data, Size size) {
    return line(std::string("add_label_with_icon_button_list|") + size_name(size) + "|" + format + "|left|" +
                button_prefix + "|" + fields + "|" + data);
}

Dialog& Dialog::start_custom_tabs() { return line("start_custom_tabs|"); }
Dialog& Dialog::end_custom_tabs() { return line("end_custom_tabs|"); }
Dialog& Dialog::quick_exit() { return line("add_quick_exit|"); }

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------
Dialog& Dialog::raw(const std::string& lines) {
    data_ += lines;
    if (!lines.empty() && lines.back() != '\n') data_ += '\n';
    return *this;
}

Dialog& Dialog::append(const Dialog& other) {
    data_ += other.data_;
    return *this;
}

Dialog& Dialog::end_dialog(const std::string& name, const std::string& cancel, const std::string& ok) {
    return line("end_dialog|" + name + "|" + cancel + "|" + ok + "|");
}

void Dialog::send(player::Player* player) const {
    if (!player) return;
    packet::Variant variant{};
    variant.add("OnDialogRequest");
    variant.add(data_);
    std::vector<std::byte> ext_data = variant.serialize();

    packet::GameUpdatePacket game_packet{};
    game_packet.type = packet::PACKET_CALL_FUNCTION;
    game_packet.net_id = static_cast<uint32_t>(-1);
    game_packet.flags.extended = 1;
    game_packet.data_size = static_cast<uint32_t>(ext_data.size());

    ByteStream<std::uint16_t> bs{};
    bs.write(packet::NET_MESSAGE_GAME_PACKET);
    bs.write(game_packet);
    bs.write_data(ext_data.data(), ext_data.size());
    (void)player->send_packet(bs.get_data(), 0);
}

}
