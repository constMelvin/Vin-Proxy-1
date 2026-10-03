#pragma once
#include <string>

namespace player { class Player; }

namespace utils {

// Growtopia dialog builder (like LuckyProxy's Dialog class).
// Every call appends one dialog line and returns *this, so calls can be chained:
//
//   utils::Dialog dlg;
//   dlg.label_with_icon("`2Auto Pull Settings``", 2246)
//      .spacer()
//      .checkbox("autopull", "`2Enable Auto Pull", enabled)
//      .description("Pulls everyone who joins the world.")
//      .button("autopull_disable", "`4Disable")
//      .end_dialog("autopull_settings", "Cancel", "Okay");
//   dlg.send(player);
//
// Values typed by the player (checkboxes, text inputs, pickers, embed_data) come back in the
// "action|dialog_return" packet as "<id>|<value>", plus "buttonClicked|<id>" for the clicked button.
//
// Elements marked [untested] are used by the game's own dialogs but not yet by this proxy.
class Dialog {
public:
    enum class Size { Big, Small };
    enum class Align { Left, Center, Right };

    // Frames for button_with_icon
    static constexpr const char* kFrameBlue = "staticBlueFrame";
    static constexpr const char* kFrameYellow = "staticYellowFrame";   // "selected" look
    static constexpr const char* kFrameNone = "noflags";

    // Colour for the "tiny grey" texts (description())
    static constexpr const char* kGreyText = "200,200,200,200";

    // ------------------------------------------------------------------
    // Dialog settings
    // ------------------------------------------------------------------
    // Default text colour of the whole dialog, e.g. "`o"
    Dialog& set_default_color(const std::string& color = "`o");
    // [untested] Background / border colour, "r,g,b,a"
    Dialog& set_bg_color(const std::string& rgba);
    Dialog& set_border_color(const std::string& rgba);
    // [untested] Spacing between button_with_icon tiles, e.g. "x:5;y:10"
    Dialog& set_custom_spacing(const std::string& spacing);
    // [untested] Width hint: button labels are sized to fit this string
    Dialog& text_scaling_string(const std::string& text);
    // Name shown by the game for this popup (server wrench menu uses "WrenchMenu")
    Dialog& popup_name(const std::string& name);

    // ------------------------------------------------------------------
    // Text
    // ------------------------------------------------------------------
    Dialog& label_with_icon(const std::string& text, int icon, Size size = Size::Big);
    // [untested] Label with an element icon (element: 0..4)
    Dialog& label_with_ele_icon(const std::string& text, int icon, int element, Size size = Size::Big);
    // Label without an icon
    Dialog& label(const std::string& text, Size size = Size::Big);
    Dialog& textbox(const std::string& text, Align align = Align::Left);
    Dialog& smalltext(const std::string& text);
    // Small text that keeps its size on every screen
    Dialog& smalltext_forced(const std::string& text);
    // Same, with alpha 0..1 (e.g. "0.5")
    Dialog& smalltext_forced_alpha(const std::string& text, const std::string& alpha);
    // Fully custom text, props e.g. "size:tiny;color:200,200,200,200"
    Dialog& custom_textbox(const std::string& text, const std::string& props);
    // Tiny grey text pulled up under the previous checkbox (same style as /spam and LuckyProxy addDescText)
    Dialog& description(const std::string& text);

    // ------------------------------------------------------------------
    // Layout
    // ------------------------------------------------------------------
    Dialog& spacer(Size size = Size::Small);
    // Moves the next element, e.g. custom_margin(0, -32)
    Dialog& custom_margin(int x, int y);
    // [untested] Starts a new row in a custom layout
    Dialog& custom_break();

    // ------------------------------------------------------------------
    // Buttons
    // ------------------------------------------------------------------
    Dialog& button(const std::string& id, const std::string& label, const std::string& flags = "noflags");
    // Label with icon that is clickable (sends buttonClicked|<button_id>)
    Dialog& label_with_icon_button(const std::string& text, int icon, const std::string& button_id);
    // Item tile button; finish a row of them with end_list()
    Dialog& button_with_icon(const std::string& id, const std::string& text, int item_id,
                             const std::string& frame = kFrameBlue, const std::string& count = "");
    // Closes a row of button_with_icon
    Dialog& end_list();
    // Image/custom button, props e.g. "image:interface/large/vin_tabs.rttex;image_size:228,92;frame:1,0;width:0.16;"
    Dialog& custom_button(const std::string& id, const std::string& props);
    // Banner image, path e.g. "interface/large/news_banner.rttex"
    Dialog& image_button(const std::string& id, const std::string& image_path,
                         const std::string& layout = "bannerlayout", const std::string& link = "");
    // Opens a website
    Dialog& url_button(const std::string& id, const std::string& label, const std::string& url);
    // Warps to a world when clicked
    Dialog& world_button(const std::string& label, const std::string& world);
    // Achievement tile
    Dialog& achieve(const std::string& title, const std::string& description, int icon);

    // ------------------------------------------------------------------
    // Inputs (values come back in dialog_return under their id)
    // ------------------------------------------------------------------
    Dialog& checkbox(const std::string& id, const std::string& label, bool checked);
    Dialog& text_input(const std::string& id, const std::string& label, const std::string& value, int max_length);
    // Input that is pre-filled and sent back as-is (used for hidden-ish values)
    Dialog& text_input_with_info(const std::string& id, const std::string& label, const std::string& value);
    // [untested] Multi-line text box
    Dialog& text_box_input(const std::string& id, const std::string& label, const std::string& value,
                           int max_length, int lines);
    // Lets the player pick an inventory item; returns "<id>|<item id>"
    Dialog& item_picker(const std::string& id, const std::string& label, const std::string& header);
    // Lets the player pick a player in the world; returns "<id>|<name>"
    Dialog& player_picker(const std::string& id, const std::string& label);
    // Hidden value sent back with the dialog_return (e.g. tile coords)
    Dialog& embed_data(const std::string& key, const std::string& value);

    // ------------------------------------------------------------------
    // Special
    // ------------------------------------------------------------------
    // Player name, level and XP bar
    Dialog& player_info(const std::string& name, int level, int xp, int xp_needed);
    // Clickable list rows filled from data, e.g. ("`w%s : %s``", "findTile_", "itemID_itemAmount", data)
    Dialog& label_with_icon_button_list(const std::string& format, const std::string& button_prefix,
                                        const std::string& fields, const std::string& data, Size size = Size::Small);
    // Image tab strip on top of the dialog: start_custom_tabs(), custom_button()..., end_custom_tabs()
    Dialog& start_custom_tabs();
    Dialog& end_custom_tabs();
    // Close button (X) in the corner
    Dialog& quick_exit();

    // ------------------------------------------------------------------
    // Building
    // ------------------------------------------------------------------
    // Raw dialog line(s); a trailing newline is added when missing
    Dialog& raw(const std::string& lines);
    Dialog& append(const Dialog& other);
    // end_dialog|name|cancel|ok| - pass "" to hide a button
    Dialog& end_dialog(const std::string& name, const std::string& cancel, const std::string& ok);

    const std::string& str() const { return data_; }
    bool empty() const { return data_.empty(); }

    // Sends the dialog to the game client as OnDialogRequest
    void send(player::Player* player) const;

    // Removes '|' and newlines so user text can't break the dialog format
    static std::string sanitize(std::string text);

private:
    Dialog& line(const std::string& text);
    static const char* size_name(Size size);
    static const char* align_name(Align align);
    std::string data_;
};

}
