# Daylight_aw themes

Since 1.1 there are two themes: the built-in **Default** ("deep space": near-black, two soft corner
halos, ring knobs) and **User**, a theme anyone can edit without rebuilding the plugin.

- `themes/default/theme.json`: the Default theme (embedded in the plugin).
- `themes/HOW_TO_MAKE_YOUR_OWN_THEME.txt`: the user guide, with every token explained. The plugin
  copies it into the user's theme folder as "How to make your own theme.txt".

## The User theme

Choose **Theme → User** in the presets + settings dropdown (click the effect name). The folder
`Documents/big_tyme_designs/Daylight_aw/Themes/User/` then holds `theme.json` (a copy of Default)
and the guide. Saved changes show up in every open window within about a second.

Themes change colours only - never sizes, positions or fonts. Anything missing or invalid uses the
Default value and the dropdown lists what was wrong, so a theme can never break the plugin.
Colours are `#RRGGBB` or `#RRGGBBAA` (AA = opacity, 00-FF). Aim for at least 4.5:1 contrast between
text and background; the dropdown warns when a theme is below that.

A User theme from Daylight_aw 1.0 (made for the old Light/Dark look, `"formatVersion": 1`) is moved,
untouched, into `Themes/User/old_1.0/` the first time 1.1 uses the User theme; nothing is deleted.
