#!/usr/bin/env python3
"""Builds docs/Daylight_aw_Manual.pdf.

  1. Pictures:  DaylightTests --manual <shots>   and   DaylightTests --controls <ctl>
                tools/manual/controls_sheet.py <ctl> <shots>/controls.png resources/fonts
  2. Manual:    tools/manual/build_manual.py <shots> docs/Daylight_aw_Manual.pdf
Needs Python 3 with playwright (Chromium) for HTML -> PDF.
"""
import asyncio, collections, html, json, pathlib, re, sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
SHOTS = pathlib.Path(sys.argv[1]).resolve()
OUT = pathlib.Path(sys.argv[2]).resolve()
F = (ROOT / "resources" / "fonts").as_posix() + "/"
VERSION = re.search(r"project\(Daylight_aw VERSION ([0-9.]+)", (ROOT / "CMakeLists.txt").read_text()).group(1)
VER_SHORT = VERSION[:-2] if VERSION.endswith(".0") else VERSION
manifest = json.loads((ROOT / "manifest" / "effects.json").read_text(encoding="utf-8"))
sel = [e for e in manifest["effects"] if e["included"]]
import datetime
_until = datetime.date.fromisoformat(manifest.get("latest", {}).get("until", "2000-01-01"))
_start = _until - datetime.timedelta(days=round(int(manifest.get("latest", {}).get("months", 6)) * 30.44))
def is_latest(e): return _start < datetime.date.fromisoformat(e["released"]) <= _until
N = len(sel)
cats = collections.OrderedDict()
for e in sorted(sel, key=lambda e: (e["category"], e["upstream"].lower())):
    cats.setdefault(e["category"], []).append(e)

css=f"""
@font-face{{font-family:Shantell;src:url('file://{F}shantell_semibold.ttf');font-weight:600}}
@font-face{{font-family:Shantell;src:url('file://{F}shantell_extrabold.ttf');font-weight:800}}
@page{{size:A4;margin:16mm 16mm 18mm 16mm}}
*{{box-sizing:border-box}}
body{{font-family:'Liberation Sans',Arial,sans-serif;font-size:10.2pt;line-height:1.45;color:#2b2a28}}
h1,h2,h3,.logo{{font-family:Shantell;font-weight:800;color:#1b1a18}}
h1{{font-size:22pt;margin:0 0 4mm}}
h2{{font-size:15pt;margin:7mm 0 2.5mm;border-bottom:1.2px solid #2b2a2844;padding-bottom:1.5mm}}
h3{{font-size:11.5pt;margin:4mm 0 1.5mm}}
p{{margin:0 0 2.2mm}}
ul,ol{{margin:0 0 2.5mm;padding-left:5mm}} li{{margin-bottom:1mm}}
table{{border-collapse:collapse;width:100%;margin:1mm 0 3mm;font-size:9.6pt}}
td,th{{border-bottom:1px solid #2b2a2826;padding:1.1mm 2mm;vertical-align:top;text-align:left}}
th{{font-family:Shantell;font-weight:600;background:#efece5}}
.k{{white-space:nowrap;font-weight:bold}}
.page{{page-break-after:always}}
.cover{{height:255mm;display:flex;flex-direction:column;justify-content:center;align-items:center;text-align:center;background:#e2dfd7;margin:-16mm -16mm 0;padding:16mm;position:relative}}
.cover .logo{{font-size:46pt;margin:0}}
.cover .sub{{font-size:13pt;margin:3mm 0 10mm;color:#3a3834}}
.cover img.shot{{width:150mm;border:1px solid #2b2a2833;border-radius:2mm;box-shadow:0 2mm 6mm #0002}}
.cover .ver{{margin-top:8mm;font-family:Shantell;font-weight:600;font-size:12pt}}
.cover .cat{{position:absolute;right:14mm;bottom:10mm;width:28mm;opacity:.9}}
.shot{{width:100%;border:1px solid #2b2a2833;border-radius:1.5mm}}
.two{{display:flex;gap:5mm}} .two>div{{flex:1}}
.cap{{font-size:8.6pt;color:#55524b;margin:1mm 0 3mm}}
.box{{background:#efece5;border-radius:2mm;padding:3mm 4mm;margin:2mm 0 3mm}}
.note{{border-left:3px solid #2b2a28;padding:1mm 3mm;margin:2mm 0 3mm;background:#f6f4ef}}
.anno{{position:relative;width:140mm;margin:0 auto}} .anno .m{{position:absolute;width:6.5mm;height:6.5mm;border-radius:50%;background:#c0392b;color:#fff;font:bold 9pt Arial;display:flex;align-items:center;justify-content:center;transform:translate(-50%,-50%);border:1.5px solid #fff}}
.fx td:first-child{{width:33mm;font-weight:bold}}
.cols{{column-count:2;column-gap:7mm}}
.small{{font-size:8.6pt;color:#55524b}}
code{{font-family:'Liberation Mono',monospace;font-size:8.8pt;background:#efece5;padding:0 1mm;border-radius:1mm}}
.draft{{position:fixed;top:2mm;right:0;font:bold 8pt Arial;color:#c0392b}}
"""
def img(name,cls='shot'): return f'<img class="{cls}" src="file://{(SHOTS / name).as_posix()}">'
marks=[(17,4,1),(52,4.5,2),(75,4.5,3),(89,4,4),(29,13,5),(36.5,24,6),(36.5,32.5,7)]
anno='<div class="anno">'+img('light.png')+''.join(f'<div class="m" style="left:{x}%;top:{y}%">{n}</div>' for x,y,n in marks)+'</div>'

h=[]
h.append(f'''<div class="cover page">
<div class="logo">Daylight_aw</div>
<div class="sub">Original Airwindows effects in a calm, hand-drawn interface</div>
{img('light.png')}
<div class="ver">Version {VER_SHORT} &middot; User manual &middot; by big_tyme_designs</div>
<img class="cat" src="file://{(ROOT / 'themes' / 'light' / 'figure.png').as_posix()}">
</div>''')

h.append(f'''
<h1>At a glance</h1>
<table>
<tr><td class="k">What</td><td>A free audio plugin that runs one original Airwindows effect per instance, in its own clear interface.</td></tr>
<tr><td class="k">Sound</td><td>The original Airwindows code by Chris Johnson, unchanged. Automatic tests prove the output is bit-identical to the original plugin.</td></tr>
<tr><td class="k">Effects</td><td>{N} effects: the newest version of every Airwindows plugin Chris recommends, plus his latest releases (list at the end).</td></tr>
<tr><td class="k">Formats</td><td>VST3 (Windows, macOS, Linux) and AU (macOS). 64-bit. Apple silicon and Intel.</td></tr>
<tr><td class="k">Systems</td><td>Windows 10/11 &middot; macOS 10.13 or newer &middot; Linux x64 (Ubuntu 22.04 or similar and newer)</td></tr>
<tr><td class="k">CPU</td><td>The same as the original Airwindows plugin. The interface only redraws what changes.</td></tr>
<tr><td class="k">Price / licence</td><td>Free and open source (GNU AGPL v3). No account, no internet connection, no tracking.</td></tr>
<tr><td class="k">Download</td><td>github.com/bigtymedesigns/daylight_aw &rarr; <b>Releases</b></td></tr>
</table>
<div class="note">Daylight_aw is <b>not an official Airwindows release</b>. All the sound is Chris Johnson's work: please support him at patreon.com/airwindows.</div>

<h2>The idea</h2>
<p>Airwindows plugins sound wonderful but come without a graphical interface: your DAW shows a row of plain sliders. Daylight_aw gives each effect a calm, readable interface, with related controls grouped, real parameter names and values, presets, A/B comparison and themes, without touching the sound.</p>
<ul>
<li><b>One effect per instance.</b> Insert Daylight_aw, choose an effect, and that instance stays that effect. Want another effect? Insert another Daylight_aw.</li>
<li><b>Nothing added to the sound.</b> The optional input/output trims are switched off completely at 0&nbsp;dB.</li>
<li><b>Safe sessions.</b> Your projects remember the effect, every setting, A/B, the trims and the preset name, and they reopen exactly as you left them.</li>
</ul>

<h2>Quick start</h2>
<ol>
<li>Install the plugin (next page) and rescan plugins in your DAW.</li>
<li>Insert <b>Daylight_aw</b> (by big_tyme_designs) on a track.</li>
<li>The effect list opens: pick a category or search, then click an effect.</li>
<li>Confirm <b>Load</b>. The window shows that effect's controls.</li>
<li>Turn the knobs. Click the effect name at the top for presets and settings.</li>
</ol>
<div class="page"></div>

<h1>Installation</h1>
<h3>Windows (VST3)</h3>
<ol><li>Close your DAW.</li>
<li>From the download zip, copy the folder <code>Daylight_aw.vst3</code> into <code>C:\\Program Files\\Common Files\\VST3\\</code> (Windows asks for administrator permission).</li>
<li>Start the DAW and rescan plugins (REAPER: Options &rarr; Preferences &rarr; Plug-ins &rarr; VST &rarr; Re-scan).</li></ol>
<h3>macOS (AU + VST3)</h3>
<ol><li>Quit your DAW. Copy <code>Daylight_aw.component</code> to <code>~/Library/Audio/Plug-Ins/Components/</code> and <code>Daylight_aw.vst3</code> to <code>~/Library/Audio/Plug-Ins/VST3/</code> (Finder: Go &rarr; Go to Folder).</li>
<li>If macOS blocks the plugin, open Terminal and run the two <code>xattr</code> lines from <code>INSTALL.txt</code> in the zip.</li>
<li>Restart the DAW and rescan (Logic: Settings &rarr; Plug-in Manager &rarr; Reset &amp; Rescan).</li></ol>
<h3>Linux (VST3)</h3>
<p>Copy <code>Daylight_aw.vst3</code> to <code>~/.vst3/</code> and rescan in your DAW.</p>

<h2>Updating</h2>
<ol><li><b>Close your DAW completely</b> (Windows can't replace a plugin file that is still loaded).</li>
<li>Replace the old <code>Daylight_aw.vst3</code> (and on Mac <code>Daylight_aw.component</code>) with the new one.</li>
<li>Start the DAW. Your projects, presets, favourites, themes and layouts are kept: they live in your Documents folder, not in the plugin.</li></ol>
<p class="small">Updates only ever add effects. An effect is never removed or renamed, so older projects always open.</p>

<h2>Uninstalling</h2>
<p>Delete <code>Daylight_aw.vst3</code> (and <code>Daylight_aw.component</code> on Mac). To remove your own data too, delete the folder <code>Documents/big_tyme_designs/Daylight_aw</code>.</p>
<div class="page"></div>

<h1>The window</h1>
{anno}
<table>
<tr><td class="k">1 Logo</td><td>Click for About: version, credits and licences.</td></tr>
<tr><td class="k">2 Effect name</td><td>Click to open <b>presets and settings</b>. Shows the preset name, with <b>*</b> when you have changed something since loading it. Trim settings other than 0&nbsp;dB are shown next to it (e.g. <i>IN +3.0</i>).</td></tr>
<tr><td class="k">3 A / B</td><td>Two complete settings to compare. Switch with one click; both are saved with the project.</td></tr>
<tr><td class="k">4 Bypass</td><td>The same switch as your DAW's bypass, with a short click-free fade.</td></tr>
<tr><td class="k">5 Groups</td><td>Related controls framed together, named after what they do.</td></tr>
<tr><td class="k">6 Controls</td><td>Knobs, stepped knobs (choices) and switches. See the next page.</td></tr>
<tr><td class="k">7 Values</td><td>In the effect's own units where it has them (dB, Hz ...).</td></tr>
</table>
<div class="two" style="align-items:flex-end"><div>{img('small.png')}<div class="cap">Effects with up to 5 controls use the small window.</div></div>
<div>{img('large.png')}<div class="cap">Effects with 6 or more controls use the large window (here StoneFireComp in the Dark theme).</div></div></div>
<div class="page"></div>

<h1>Controls and mouse</h1>
<table>
<tr><th>Do this</th><th>To</th></tr>
<tr><td class="k">Drag up / down</td><td>Turn a knob (faders in the Original theme: drag sideways)</td></tr>
<tr><td class="k">Shift + drag</td><td>Fine adjustment (10&times; finer)</td></tr>
<tr><td class="k">Mouse wheel</td><td>Small steps (Shift: finer)</td></tr>
<tr><td class="k">Double-click a knob</td><td>Back to the effect's default</td></tr>
<tr><td class="k">Double-click the value</td><td>Type an exact value, Enter to confirm</td></tr>
<tr><td class="k">Click a switch</td><td>Toggle it</td></tr>
<tr><td class="k">Hover</td><td>Underlines the name and shows one more decimal; with tooltips on, a hint and the original Airwindows name</td></tr>
</table>
{img('controls.png')}
<div class="cap">Knob, stepped knobs and switch in the Light and Dark themes, plus the trim knobs from Settings.</div>
<p>Everything is automatable. Your DAW shows the effect's real parameter names.</p>
<div class="page"></div>

<h1>Choosing an effect</h1>
<div class="two"><div style="flex:1.25">{img('selector.png')}</div><div>
<ul>
<li><b>Favourites</b>: right-click an effect or click its star.</li>
<li><b>Recently used</b>: your last 10.</li>
<li><b>Latest</b>: the newest Airwindows releases (the 6 months before this Daylight_aw version).</li>
<li><b>All effects</b>, and Airwindows' own <b>categories</b>.</li>
<li><b>Search</b> looks in names and descriptions.</li>
<li>Hover an effect to read what it does.</li>
</ul>
<p>The choice is locked for that instance, so a project can never change sound by accident. Favourites and recently used are shared by every Daylight_aw instance.</p>
</div></div>

<h2>Presets</h2>
<div class="two"><div style="flex:1.25">{img('settings.png')}</div><div>
<ul>
<li>Click the effect name &rarr; <b>Save preset</b>, type a name, Enter.</li>
<li>Click a preset to load it (into the current A or B).</li>
<li>Right-click: rename, move to trash, show in folder.</li>
<li>Presets are per effect, stored as small text files you can copy or share.</li>
<li>Presets don't store trims or bypass.</li>
</ul></div></div>
<div class="page"></div>

<h1>Settings</h1>
<table>
<tr><td class="k">Zoom</td><td>100, 125, 150 or 200&nbsp;% (for all instances).</td></tr>
<tr><td class="k">Theme</td><td><b>Light</b> (default), <b>Dark</b>, <b>Original</b> or <b>User</b>. The choice applies to every open instance.</td></tr>
<tr><td class="k">Trims</td><td>Input trim (before the effect) and output trim (after it), &plusmn;12&nbsp;dB, for gain staging. Double-click for 0&nbsp;dB, where they are switched off completely. On dither effects, keep the output trim at 0&nbsp;dB.</td></tr>
<tr><td class="k">Tooltips</td><td>Off by default. Switch on for hints when hovering.</td></tr>
<tr><td class="k">Reset to defaults</td><td>All controls back to the effect's defaults.</td></tr>
<tr><td class="k">Effect info</td><td>The full Airwindopedia text by Chris about this effect.</td></tr>
<tr><td class="k">Edit layout</td><td>Make your own layout (page "Make it yours").</td></tr>
</table>
<div class="two"><div>{img('dark.png')}<div class="cap">Dark theme</div></div><div>{img('original.png')}<div class="cap">Original theme: plain faders, Airwindows' own names and values, the computer's own font</div></div></div>
<div style="width:115mm;margin:0 auto">{img('info.png')}<div class="cap">Effect info: the Airwindopedia text for the effect.</div></div>
<div class="page"></div>

<h1>Make it yours</h1>
<h3>Your own colours: the User theme</h3>
<ol><li>Settings &rarr; Theme &rarr; <b>User</b>, then <b>Open theme folder</b>.</li>
<li>Edit <code>theme.json</code> (colours like <code>"paper.color": "#E2DFD7"</code>) or replace <code>paper.png</code> / <code>figure.png</code>.</li>
<li>Save: the window updates within a second. Mistakes fall back to safe values, nothing can break.</li></ol>
<h3>Your own layouts</h3>
<ol><li>Open an effect &rarr; Settings &rarr; <b>Edit layout</b>. A file <code>&lt;effect&gt;.json</code> appears in the Layouts folder, as a copy of what you see.</li>
<li>Open it in any text editor: rename groups, move controls, change labels or control types. Save, and the window updates.</li>
<li><b>Remove</b> goes back to the built-in layout. Typos are fixed automatically and every control always stays available.</li></ol>
<div class="box"><b>Layout file example</b><br><code>{{ "groups": [ {{ "title": "Tone", "controls": [ "Tilt", {{ "param": "HeadBmp", "label": "Bump" }} ] }} ] }}</code></div>

<h2>Where your files are</h2>
<table>
<tr><th>What</th><th>Folder (inside <code>Documents/big_tyme_designs/Daylight_aw/</code>)</th></tr>
<tr><td>Presets</td><td><code>Presets/&lt;Effect&gt;/&lt;name&gt;.dlpreset</code></td></tr>
<tr><td>User theme</td><td><code>Themes/User/</code></td></tr>
<tr><td>Your layouts</td><td><code>Layouts/&lt;effect&gt;.json</code></td></tr>
<tr><td>Settings (theme, zoom, favourites)</td><td><code>settings.json</code></td></tr>
</table>
<p class="small">Back up this folder to keep everything when you move to a new computer.</p>
<div class="page"></div>

<h1>Troubleshooting &amp; FAQ</h1>
<table>
<tr><td class="k">Daylight_aw doesn't show up</td><td>Check the plugin is in the right folder (page 3) and rescan. On Mac, run the <code>xattr</code> lines from INSTALL.txt.</td></tr>
<tr><td class="k">"Access denied" when updating (Windows)</td><td>The DAW still has the plugin loaded. Close it completely; if needed end <i>reaper.exe</i> / <i>reaper_host</i> in Task Manager. No restart needed.</td></tr>
<tr><td class="k">"This effect needs a newer Daylight_aw"</td><td>The project was saved with a newer version that has more effects. Update Daylight_aw. Until then audio passes through untouched and the settings are kept.</td></tr>
<tr><td class="k">Does it sound exactly like the original?</td><td>Yes. Every effect is tested against the original code for bit-identical output (trims at 0&nbsp;dB, not bypassed).</td></tr>
<tr><td class="k">Can I change the effect of an instance?</td><td>No, by design: insert another Daylight_aw instead. This keeps projects safe.</td></tr>
<tr><td class="k">Does it use the internet?</td><td>Never. No accounts, no update checks, no tracking.</td></tr>
<tr><td class="k">Reverb tail cut off?</td><td>Reverbs report their tail to the DAW, so rendering and pausing keep it. If your DAW still cuts it, render with extra tail time.</td></tr>
<tr><td class="k">Found a bug, or have an idea?</td><td>Open an <b>Issue</b> on the GitHub page. Effects, layouts and themes can also be contributed there (see CONTRIBUTING.md).</td></tr>
</table>

<h2>Legal &amp; credits</h2>
<ul>
<li><b>Daylight_aw</b> &copy; big_tyme_designs. Free software under the GNU Affero General Public License v3. The source code is at github.com/bigtymedesigns/daylight_aw. It comes <b>without any warranty</b>.</li>
<li><b>Airwindows</b> effects and Airwindopedia texts &copy; Chris Johnson, MIT licence. Packaged via <b>Airwindows Consolidated</b> by Paul Walker (MIT). Daylight_aw is not an official Airwindows release.</li>
<li><b>JUCE</b> framework (AGPLv3). <b>VST</b> is a registered trademark of Steinberg Media Technologies GmbH. <b>Audio Units</b> and macOS are trademarks of Apple Inc.</li>
<li><b>Shantell Sans</b> font by Shantell Martin / Arrow Type, SIL Open Font Licence 1.1.</li>
<li>Full licence texts: <code>LICENSE</code> and <code>THIRD_PARTY_NOTICES.md</code> in the download.</li>
</ul>
<div class="page"></div>
''')
h.append(f'<h1>The effects ({N})</h1><p class="small">Grouped by Airwindows category, alphabetically. Descriptions by Chris Johnson / Airwindows. Effects marked &#9733; are in the Latest folder.</p><div class="cols">')
for cat,items in cats.items():
    h.append(f'<h3>{html.escape(cat)}</h3><table class="fx">'+''.join(f'<tr><td>{html.escape(e["upstream"])}{" &#9733;" if is_latest(e) else ""}</td><td>{html.escape(e["description"])}</td></tr>' for e in items)+'</table>')
h.append('</div>')
doc=f'<html><head><meta charset="utf-8"><style>{css}</style></head><body>{"".join(h)}</body></html>'
html_file = OUT.with_suffix(".html")
html_file.write_text(doc, encoding="utf-8")

async def render():
    from playwright.async_api import async_playwright
    async with async_playwright() as p:
        b = await p.chromium.launch()
        pg = await b.new_page()
        await pg.goto(html_file.as_uri()); await pg.wait_for_timeout(500)
        await pg.pdf(path=str(OUT), format="A4", print_background=True, display_header_footer=True,
                     header_template="<span></span>",
                     footer_template=f'<div style="font-size:7pt;width:100%;text-align:center;color:#888">Daylight_aw {VER_SHORT} manual &middot; <span class="pageNumber"></span></div>',
                     margin={"top": "16mm", "bottom": "18mm", "left": "16mm", "right": "16mm"})
        await b.close()

asyncio.run(render())
html_file.unlink()
print(f"manual: {OUT} ({N} effects, version {VERSION})")
