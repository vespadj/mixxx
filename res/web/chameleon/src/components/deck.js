import * as decksApi from "../api/decks.js";

/**
 * Deck component — deck state management, transport controls, polling.
 *
 * Methods are designed to be mixed into the main Alpine component (they use `this`).
 */
export default {
    // ─── Deck polling ──────────────────────────────────────

    startDeckPolling() {
        if (this._deckPollStarted) { return; }
        this._deckPollStarted = true;

        // Poll all visible decks every second (when mixer or autodj tab is visible)
        this._deckPollTimer = setInterval(() => {
            if (!this.deckSeeking && (this.activeTab === "mixer" || this.activeTab === "autodj")) {
                for (const deckId of this.settings.visibleDecks.map(d => d.id)) {
                    this.getDeckState(deckId);
                }
            }
        }, 1000);
    },

    onDeckChange(deckId) {
        this.selectedDeck = deckId;
        this.getDeckState(deckId);
    },

    async getDeckState(deck) {
        try {
            const res = await decksApi.getDeckState(deck);
            for (const item of res) {
                if (item.playing !== undefined) {
                    this.Decks[deck].play = item.playing;
                }
                if (item.duration !== undefined) {
                    this.Decks[deck].duration = item.duration;
                    if (deck === this.selectedDeck) {
                        this.deckDuration = item.duration;
                    }
                }
                if (item.position !== undefined && !this.deckSeeking) {
                    this.Decks[deck].pos = item.position;
                    this.Decks[deck].elapsed = item.elapsed || 0;
                }
                if (item.title !== undefined) {
                    this.Decks[deck].title = item.title || "–";
                }
                if (item.artist !== undefined) {
                    this.Decks[deck].artist = item.artist || "";
                }
                if (item.key !== undefined) {
                    this.Decks[deck].key = item.key || "–";
                }
                if (item.bpm !== undefined) {
                    this.Decks[deck].bpm = item.bpm;
                }
            }
        } catch (err) {
            console.error("getDeckState error:", err);
        }
    },

    onDeckSeekInput(sliderValue) {
    // Update elapsed display only while dragging slider
        this.Decks[this.selectedDeck].pos = sliderValue / 1000;
    },

    async onDeckSeekCommit(sliderValue) {
        await decksApi.setDeckPosition(this.selectedDeck, sliderValue / 1000);
    },

    /**
     * Play/Pause toggle — triggered on press (mousedown/touchstart), Pioneer CDJ style.
     * @param deckId
     */
    async deckPlayPress(deckId) {
        const playing = !this.Decks[deckId].play;
        await decksApi.setDeckPlay(deckId, playing);
        this.Decks[deckId].play = playing;
    },

    /**
     * Play button mouseup — Pioneer CDJ "drag from CUE to PLAY" emulation.
     * If CUE is still pressed (finger dragged from CUE to PLAY), start playback
     * and release CUE simultaneously, just like pressing PLAY with another
     * finger while holding CUE on a real CDJ.
     * @param deckId
     */
    async deckPlayUp(deckId) {
        const deck = this.Decks[deckId];
        if (!deck) { return; }
        if (deck.cuePressed) {
            const group = `[Channel${deckId}]`;
            // Setting play=1 automatically resets cue_default,
            // so no explicit cue_default=0 is needed here.
            await this.setParam(group, "play", 1);
            alert('component deck');
            deck.play = true;
            deck.cuePressed = false;
        }
        // If cuePressed is false, this was a normal click — mousedown already
        // toggled play, so do nothing on mouseup.
    },

    async deckStop(deckId) {
        await decksApi.deckStop(deckId);
        this.Decks[deckId].play = false;
    },

    /**
     * CUE press — Pioneer CDJ style: preview from current position (cue_default),
     * or jump to cue point if near end of track (cue_goto).
     * @param deckId
     */
    async deckCuePress(deckId) {
        const deck = this.Decks[deckId];
        if (!deck) { return; }
        deck.cuePressed = true;
        const group = `[Channel${deckId}]`;
        if (deck.pos > 0.97) {
            await this.setParam(group, "cue_goto", 1);
        } else {
            await this.setParam(group, "cue_default", 1);
        }
    },

    /**
     * CUE release — sends the corresponding 0 to release the cue button.
     * @param deckId
     */
    async deckCueRelease(deckId) {
        const deck = this.Decks[deckId];
        if (!deck || !deck.cuePressed) { return; }
        deck.cuePressed = false;
        const group = `[Channel${deckId}]`;
        if (deck.pos > 0.97) {
            await this.setParam(group, "cue_goto", 0);
        } else {
            await this.setParam(group, "cue_default", 0);
        }
    },

    /**
     * SYNC press — Pioneer CDJ style with tap/hold logic.
     * If not sync leader: enable sync, record timestamp.
     * If already sync leader: disable sync, clear timestamp.
     * @param deckId
     */
    async deckSyncPress(deckId) {
        const deck = this.Decks[deckId];
        if (!deck) { return; }
        const group = `[Channel${deckId}]`;
        if (!deck.syncLeader) {
            await this.setParam(group, "sync_enabled", 1);
            deck.syncLastTimestamp = Date.now();
        } else {
            await this.setParam(group, "sync_enabled", 0);
            deck.syncLastTimestamp = 0;
        }
    },

    /**
     * SYNC release — Pioneer CDJ style:
     * Short tap (< 250ms): disable sync (momentary).
     * Long press (>= 250ms): keep sync leader active.
     * @param deckId
     */
    async deckSyncRelease(deckId) {
        const deck = this.Decks[deckId];
        if (!deck) { return; }
        if (deck.syncLastTimestamp === 0) { return; }
        const group = `[Channel${deckId}]`;
        if (Date.now() - deck.syncLastTimestamp < 250) {
            await this.setParam(group, "sync_enabled", 0);
        }
        // else: long press — keep sync leader active, do nothing
    },

    async loadDeck(trackId, deck, play) {
        await decksApi.loadDeck(trackId, deck, play);
    },

    // ─── Playing decks (for AutoDJ tab) ────────────────────

    /**
     * Returns an array of decks that are currently playing.
     * Used in the AutoDJ tab to show now‑playing info.
     */
    get playingDecks() {
        return this.settings.decks
            .map(sd => this.Decks[sd.id])
            .filter(d => d && d.play);
    },

    /**
     * Cycle the time display mode for a deck.
     * Cycles: 'elapsed' → 'remaining' → 'both' → 'elapsed'
     * @param deckId
     */
    cycleTimeDisplay(deckId) {
        const deck = this.Decks[deckId];
        if (!deck) { return; }
        const modes = ["elapsed", "remaining", "both"];
        const idx = modes.indexOf(deck.timeDisplayMode);
        deck.timeDisplayMode = modes[(idx + 1) % modes.length];
        // Persist to settings
        this.settings.decks = this.settings.decks.map(d =>
            d.id === deckId ? {...d, timeDisplayMode: deck.timeDisplayMode} : d
        );
    },

    /**
     * Format the time display for a deck based on its timeDisplayMode.
     * @param {number} deckId
     * @returns {string} formatted time string
     */
    formatDeckTime(deckId) {
        const deck = this.Decks[deckId];
        if (!deck || !deck.duration) { return "0:00"; }
        const elapsed = deck.elapsed || 0;
        const remaining = deck.duration - elapsed;
        switch (deck.timeDisplayMode) {
        case "elapsed":
            return this.formatTime(elapsed);
        case "remaining":
            return `-${  this.formatTime(remaining)}`;
        case "both":
            return `${this.formatTime(elapsed)  } / -${  this.formatTime(remaining)}`;
        default:
            return this.formatTime(elapsed);
        }
    },
};
