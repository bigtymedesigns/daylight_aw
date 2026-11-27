# Stage 1.1 – decision log

Short log for decisions made during the 1.1 build. Merged into `CHANGELOG.md` / the plan at stage close, then archived.
Design decisions from before the build: `Daylight_1.1_Plan.md`.

- 2026-11-26 – 1.1 is built in Claude Code on Johan's PC; `CLAUDE.md` added; project docs live in `docs/claude/` (repo is the master).
- 2026-11-26 – No second private build. One build with all effects + browser view "Latest versions only / All versions" (manifest `family`); Favourites, collections and Recently used always show their effects, older versions included (change list item 11).
- 2026-11-26 – Johan: build 1.1 ("add all of these and build"). Open questions taken as recommended: Q1 Inter for UI, Shantell removed · Q2 no paper/figure images · Q3 old User theme → `old_1.0/` · Q4 upstream check first, list shown before adding · Q5 release date set at release.
- 2026-11-26 – Families: name without trailing number; Desk4, kPlate140, kPlate240 are their own family (manifest `family`). 425 families. ConsoleXChannel/ConsoleX2Channel stay separate (number inside the name).
- 2026-11-26 – Latest view pins older versions that are a favourite or in a collection everywhere (categories, search), not only in Favourites. Switch = check box right of the list heading (no mockup; easy to move).
- 2026-11-26 – Small window: the presets + settings dropdown uses a compact layout (labels beside Zoom/Theme) so everything fits in 240 px.
- 2026-11-26 – Upstream (airwin2rack 91b3343): no code changes for included effects; new: PunchyDeluxe2, PointyDeluxe2 (not added yet, Johan decides).
- 2026-11-26 – Johan (Yes): Change effect in the dropdown (confirmation; new effect at defaults; trims/bypass kept; a state of another effect switches to it = DAW undo). Release date on every browser row. A/B, page tab, bypass-on green (#2FC4A5, dark text). About 180 px, 12/10/9 px, without "Based on the work of Chris Johnson". PunchyDeluxe2 + PointyDeluxe2 added (since 1.1.0; PointyDeluxe2 in Amp Sims, description "A new version of PointyDeluxe."); pin 91b3343.
