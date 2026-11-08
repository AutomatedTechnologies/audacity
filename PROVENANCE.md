# Provenance — AutomatedTechnologies/audacity

This repository is a fork, held by the **Detroit Automation Academy** (AutomatedTechnologies) as the base for **SoundDeck** (Project 05 of the Academy's macOS Legacy Uplift Program) — an Audacity-derived audio editor combined with libVLC playback/conversion, built for the program's macOS High Sierra 10.13 floor.

## Upstream

- **Project:** Audacity — the free, open-source audio editor, by the Audacity Team and its contributors.
- **Upstream repository:** https://github.com/audacity/audacity
- **License:** GNU General Public License (the GPL text ships in this repository as LICENSE.txt). The Academy's Gate-0 record for SoundDeck pins the exact grant before any combined work ships; GitHub's license detection reports NOASSERTION for this repository, so no more specific version claim is made here than the license file itself supports.
- **Fork point:** upstream `master` at commit `b98c8828e9917641f60b0bc8a0ad6e51f6a87ab0` — this fork's `master` was byte-identical to upstream at that commit; the only Academy commits on `master` are this provenance acknowledgment itself. All Academy work happens on Academy branches, and upstream history is never rewritten.
- **Fork taken:** 2026-11-08.

## Academy use

- **Build base for High Sierra:** the Audacity **3.7.9** release line in this history (the last line carrying the program's 10.13 floor) — pinned exactly at the SoundDeck build tranche. Upstream's current development (the Audacity 4 / Qt line) requires newer macOS and is not the Academy's base.
- **Identity:** the Academy product is **SoundDeck**, its own name and bundle identity (`com.detroitautomationacademy.sounddeck`) — no upstream trademarks are carried into the product. Upstream identifiers remain in upstream's own builds only.
- **Terms honored in full:** GPL source + attribution obligations ship with every Academy binary, per the program's standing no-lawsuits rule.

All credit for the original work belongs to its authors.
