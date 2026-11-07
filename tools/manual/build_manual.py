#!/usr/bin/env python3
"""Builds docs/Daylight_aw_Manual.pdf.

  1. Pictures:  DaylightTests --manual <shots>
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
N = len(sel)

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
.title{{display:flex;align-items:baseline;justify-content:space-between;border-bottom:2px solid #1b1a18;margin-bottom:4mm;padding-bottom:2mm}}
.title .logo{{font-size:26pt}} .title .ver{{font-family:Shantell;font-weight:600;font-size:11pt}}
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
# markers on effect.png (percent of the picture): logo, effect name, A/B, bypass, group, knob, value
marks=[(16.2,5.2,1),(53.5,5.2,2),(77.6,5.2,3),(83.3,5.2,4),(30.8,13.4,5),(36.2,21.5,6),(36.2,32.5,7)]
anno='<div class="anno">'+img('effect.png')+''.join(f'<div class="m" style="left:{x}%;top:{y}%">{n}</div>' for x,y,n in marks)+'</div>'

h=[]
h.append(f'''
<div class="title"><div class="logo">Daylight_aw</div><div class="ver">Version {VER_SHORT} &middot; Instruction manual</div></div>
<p>Daylight_aw is a free audio plugin that runs one original Airwindows effect per instance. It contains {N} effects by Chris Johnson (Airwindows), from Airwindows Consolidated. The sound is the original code, unchanged and bit-identical to the original plugins.</p>
<table>
<tr><td class="k">Formats</td><td>VST3 (Windows, macOS, Linux), AU (macOS). 64-bit, Apple silicon and Intel.</td></tr>
<tr><td class="k">Systems</td><td>Windows 10/11 &middot; macOS 10.13 or newer &middot; Linux x64</td></tr>
<tr><td class="k">Licence</td><td>Free and open source (GNU AGPL v3). No account, no internet connection.</td></tr>
</table>
<p class="small">Daylight_aw is not an official Airwindows release. Designed by big_tyme_designs.</p>

<h2>Installation</h2>
<table>
<tr><td class="k">Windows</td><td>Close your DAW. Copy <code>Daylight_aw.vst3</code> to <code>C:\\Program Files\\Common Files\\VST3\\</code>. Start the DAW and rescan plugins.</td></tr>
<tr><td class="k">macOS</td><td>Quit your DAW. Copy <code>Daylight_aw.component</code> to <code>~/Library/Audio/Plug-Ins/Components/</code> and <code>Daylight_aw.vst3</code> to <code>~/Library/Audio/Plug-Ins/VST3/</code>. If macOS blocks the plugin, run the two <code>xattr</code> lines from <code>INSTALL.txt</code> in Terminal. Restart the DAW and rescan.</td></tr>
<tr><td class="k">Linux</td><td>Copy <code>Daylight_aw.vst3</code> to <code>~/.vst3/</code> and rescan.</td></tr>
<tr><td class="k">Updating</td><td>Close the DAW completely, then replace the plugin file(s). Projects, presets and settings are kept. Effects are never removed or renamed, so older projects always open.</td></tr>
<tr><td class="k">Uninstalling</td><td>Delete the plugin file(s). Your presets and settings stay in the folder listed under <i>Your files</i>.</td></tr>
</table>

<h2>Getting started</h2>
<ol>
<li>Insert <b>Daylight_aw</b> on a track. The effect browser opens.</li>
<li>Choose a folder or type in the search field, then click an effect and confirm with <b>Load effect</b>.</li>
<li>The window now shows that effect's controls. Click the effect name at the top for presets and settings.</li>
</ol>
<p>An instance keeps its effect for good, so a project can never change sound by accident. For another effect, insert another Daylight_aw.</p>
<div class="page"></div>

<h1>The window</h1>
<div style="width:150mm;margin:0 auto">{anno}</div>
<table>
<tr><td class="k">1 Name</td><td>Click for version, credits and licences.</td></tr>
<tr><td class="k">2 Effect / preset</td><td>Click to open presets and settings. Shows the preset name, with <b>*</b> when changed since loading. Trims other than 0&nbsp;dB are shown here (e.g. <i>IN +3.0</i>).</td></tr>
<tr><td class="k">3 A / B</td><td>Two complete settings to compare. Both are saved with the project.</td></tr>
<tr><td class="k">4 Bypass</td><td>The same switch as your DAW's bypass, with a short click-free fade.</td></tr>
<tr><td class="k">5 Group</td><td>Related controls, named after what they do.</td></tr>
<tr><td class="k">6 Control</td><td>Knob, stepped knob (choices) or switch. Effects with more than 15 controls have numbered pages at the bottom.</td></tr>
<tr><td class="k">7 Value</td><td>In the effect's own units where it has them (dB, Hz ...).</td></tr>
</table>

<h2>Mouse</h2>
<table>
<tr><td class="k">Drag up / down</td><td>Turn a knob</td></tr>
<tr><td class="k">Shift + drag</td><td>Fine adjustment (10&times; finer)</td></tr>
<tr><td class="k">Mouse wheel</td><td>Small steps (Shift: finer)</td></tr>
<tr><td class="k">Double-click a knob</td><td>Back to the default value</td></tr>
<tr><td class="k">Double-click a value</td><td>Type an exact value, Enter to confirm</td></tr>
<tr><td class="k">Click a switch</td><td>Toggle it</td></tr>
</table>
<p>Every control can be automated. Your DAW shows the effect's own parameter names.</p>
<div class="page"></div>

<h1>Effect browser</h1>
<div style="width:105mm;margin:0 auto 3mm">{img('selector.png')}</div>
<table>
<tr><td class="k">Favourites</td><td>Right-click an effect or click its star to add or remove it.</td></tr>
<tr><td class="k">Recently used</td><td>Your last 10 effects.</td></tr>
<tr><td class="k">New</td><td>Airwindows releases from the {manifest["release"]["newMonths"]} months before this Daylight_aw version.</td></tr>
<tr><td class="k">Last added</td><td>Effects added in the latest Daylight_aw update. Empty in version 1.0.</td></tr>
<tr><td class="k">All effects</td><td>Every effect, A to Z, followed by the Airwindows categories.</td></tr>
<tr><td class="k">Search</td><td>Looks in effect names and descriptions.</td></tr>
</table>
<p>Hover an effect to read what it does. Favourites and recently used are shared by all instances.</p>

<h2>Presets and settings</h2>
<table>
<tr><td class="k">Save preset</td><td>Type a name and press Enter. Click a preset to load it into the current A or B. Right-click to rename, move to trash or show it in its folder. Presets belong to one effect and don't include trims or bypass.</td></tr>
<tr><td class="k">Zoom</td><td>100, 125, 150 or 200&nbsp;%, for all instances.</td></tr>
<tr><td class="k">Trims</td><td>Input trim (before the effect) and output trim (after it), &plusmn;12&nbsp;dB, for gain staging. At 0&nbsp;dB they are switched off completely. Keep the output trim at 0&nbsp;dB on dither effects.</td></tr>
<tr><td class="k">Tooltips</td><td>Hints and the original Airwindows parameter names when hovering. Off by default.</td></tr>
<tr><td class="k">Reset to defaults</td><td>All controls of the effect back to their defaults.</td></tr>
<tr><td class="k">Effect info</td><td>The full Airwindopedia text by Chris Johnson about the effect.</td></tr>
</table>
<div class="page"></div>

<h1>Your files</h1>
<p>Presets and settings are stored in <code>Documents/big_tyme_designs/Daylight_aw/</code>. Back up this folder to keep them when you move to a new computer.</p>
<table>
<tr><td class="k">Presets</td><td><code>Presets/&lt;Effect&gt;/&lt;name&gt;.dlpreset</code></td></tr>
<tr><td class="k">Settings</td><td><code>settings.json</code> (zoom, favourites, recently used)</td></tr>
</table>

<h2>Troubleshooting</h2>
<table>
<tr><td class="k">Plugin doesn't show up</td><td>Check the plugin folder (Installation) and rescan. On a Mac, run the <code>xattr</code> lines from INSTALL.txt.</td></tr>
<tr><td class="k">"Access denied" when updating (Windows)</td><td>The DAW still has the plugin loaded. Close it completely, then try again.</td></tr>
<tr><td class="k">"This effect needs a newer Daylight_aw"</td><td>The project was saved with a newer version. Update Daylight_aw. Until then, audio passes through untouched and the settings are kept.</td></tr>
<tr><td class="k">Reverb tail cut off</td><td>Reverbs report their tail to the DAW. If a render still cuts it, add tail time in the render settings.</td></tr>
<tr><td class="k">Bugs and ideas</td><td>Open an Issue at github.com/bigtymedesigns/daylight_aw.</td></tr>
</table>

<h2>Legal and credits</h2>
<ul class="small">
<li><b>Daylight_aw</b>: free software under the GNU Affero General Public License v3, without any warranty. Source code: github.com/bigtymedesigns/daylight_aw.</li>
<li><b>Airwindows</b> effects and Airwindopedia texts &copy; Chris Johnson, MIT licence, via <b>Airwindows Consolidated</b> by Paul Walker (MIT). Support Chris at patreon.com/airwindows.</li>
<li><b>JUCE</b> (AGPLv3). <b>VST</b> is a registered trademark of Steinberg Media Technologies GmbH. <b>Audio Units</b> and macOS are trademarks of Apple Inc.</li>
<li><b>Shantell Sans</b> font, SIL Open Font Licence 1.1. Full licence texts: <code>LICENSE</code> and <code>THIRD_PARTY_NOTICES.md</code>.</li>
</ul>
''')
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
                     footer_template=f'<div style="font-size:7pt;width:100%;text-align:center;color:#888">Daylight_aw {VER_SHORT} &middot; <span class="pageNumber"></span></div>',
                     margin={"top": "16mm", "bottom": "18mm", "left": "16mm", "right": "16mm"})
        await b.close()

asyncio.run(render())
html_file.unlink()
print(f"manual: {OUT} ({N} effects, version {VERSION})")
