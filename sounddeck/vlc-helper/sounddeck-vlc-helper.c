/*
 * sounddeck-vlc-helper.c — SoundDeck SD-T4 libVLC helper prototype.
 *
 * ADDED BY DETROIT AUTOMATION ACADEMY (2026-11-09, SoundDeck tranche
 * SD-T4 / D3 combine). This file is NOT part of upstream Audacity and was
 * not present in the Audacity-3.7.9 tree at the fork pin
 * (86d74c770974b25188ca2f23bcce47c1181bd08a); it is a new Academy file,
 * recorded here so the addition is marked as such per Gate 0 Condition 5
 * (GPLv3 sec. 5 — added files identified, upstream files untouched).
 *
 * Copyright (C) 2026 Detroit Automation Academy / Automated Technologies.
 * License: GNU General Public License version 3 — the license the
 * combined SoundDeck work ships under (SoundDeck files/02 Gate 0 memo).
 * This program uses libVLC (LGPL-2.1-or-later) through its public API
 * only; it contains no VLC application code (Gate 0 Condition 2).
 *
 * What this is: the build-level prototype of the SoundDeck media-engine
 * helper process for SD-T4. It proves, against libVLC 3.0.24 built from
 * the pinned VLC tag (files/03), that the pinned Audacity-line tree can
 * load libVLC, play media, transcode media, and extract audio from an
 * A/V file — each verdict read from libVLC runtime state, not from any
 * configuration text (the SD-T3 lesson: verify the binary, not the
 * cache). Modes:
 *
 *   --version                      print libvlc_get_version(), exit 0
 *   --probe-play <file>            play to Ended (dummy aout/vout);
 *                                  exit 0 iff state reaches libvlc_Ended
 *   --probe-transcode <in> <out.wav>
 *                                  sout transcode to s16l WAV; exit 0 on Ended
 *   --probe-extract <in> <out.wav> sout transcode with vcodec=none
 *                                  (audio-only extraction); exit 0 on Ended
 *   --serve                        line protocol on stdin/stdout driving
 *                                  one libvlc_media_player_t:
 *                                  LOAD <path> / PLAY / PAUSE / SEEK <ms> /
 *                                  TIME / STOP / QUIT — every reply is
 *                                  prefixed "OK" or "ERR"; TIME replies
 *                                  "OK TIME <ms> <state-name>".
 *
 * All probe/serve instances are created with:
 *   --ignore-config --audio-output=dummy --video-output=dummy
 *   --no-metadata-network
 * (files/04 sec. 1: the helper must not initiate network access the user
 * did not ask for; --no-metadata-network disables libVLC metadata/art
 * fetching, and the dummy outputs keep the proof headless.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <vlc/vlc.h>

#define PROBE_TIMEOUT_MS 60000
#define POLL_STEP_MS 100

static const char *state_name(libvlc_state_t s)
{
    switch (s) {
    case libvlc_NothingSpecial: return "nothing-special";
    case libvlc_Opening:        return "opening";
    case libvlc_Buffering:      return "buffering";
    case libvlc_Playing:        return "playing";
    case libvlc_Paused:         return "paused";
    case libvlc_Stopped:        return "stopped";
    case libvlc_Ended:          return "ended";
    case libvlc_Error:          return "error";
    }
    return "unknown";
}

static libvlc_instance_t *make_instance(void)
{
    static const char *const args[] = {
        "--ignore-config",
        "--audio-output=dummy",
        "--video-output=dummy",
        "--no-metadata-network",
    };
    libvlc_instance_t *inst = libvlc_new((int)(sizeof(args) / sizeof(args[0])), args);
    if (!inst)
        fprintf(stderr, "ERR libvlc_new failed\n");
    return inst;
}

/* Play the given media on a fresh player; poll state until Ended/Error or
 * timeout. Prints the state trace. Returns 0 iff libvlc_Ended is reached. */
static int play_to_end(libvlc_instance_t *inst, libvlc_media_t *media,
                       const char *label)
{
    (void)inst; /* the player is created from the media, which owns the ref */
    libvlc_media_player_t *mp = libvlc_media_player_new_from_media(media);
    if (!mp) {
        fprintf(stderr, "ERR %s: libvlc_media_player_new_from_media failed\n", label);
        return 1;
    }
    if (libvlc_media_player_play(mp) != 0) {
        fprintf(stderr, "ERR %s: libvlc_media_player_play failed\n", label);
        libvlc_media_player_release(mp);
        return 1;
    }
    libvlc_state_t last = libvlc_NothingSpecial;
    int waited = 0;
    int rc = 1;
    for (;;) {
        libvlc_state_t st = libvlc_media_player_get_state(mp);
        if (st != last) {
            printf("STATE %s %s\n", label, state_name(st));
            fflush(stdout);
            last = st;
        }
        if (st == libvlc_Ended) { rc = 0; break; }
        if (st == libvlc_Error) { rc = 1; break; }
        if (waited >= PROBE_TIMEOUT_MS) {
            fprintf(stderr, "ERR %s: timeout after %d ms (last state %s)\n",
                    label, PROBE_TIMEOUT_MS, state_name(last));
            rc = 1;
            break;
        }
        usleep(POLL_STEP_MS * 1000);
        waited += POLL_STEP_MS;
    }
    printf("RESULT %s %s\n", label, rc == 0 ? "ENDED" : "FAILED");
    libvlc_media_player_stop(mp);
    libvlc_media_player_release(mp);
    return rc;
}

static int probe_play(const char *path)
{
    libvlc_instance_t *inst = make_instance();
    if (!inst) return 1;
    libvlc_media_t *media = libvlc_media_new_path(inst, path);
    if (!media) {
        fprintf(stderr, "ERR libvlc_media_new_path failed for %s\n", path);
        libvlc_release(inst);
        return 1;
    }
    int rc = play_to_end(inst, media, "play");
    libvlc_media_release(media);
    libvlc_release(inst);
    return rc;
}

/* Transcode (or audio-extract) input to a 44.1 kHz stereo s16 WAV via the
 * libVLC stream-output chain. extract != 0 adds vcodec=none so only the
 * audio elementary stream reaches the WAV muxer. */
static int probe_sout(const char *in_path, const char *out_path, int extract)
{
    libvlc_instance_t *inst = make_instance();
    if (!inst) return 1;
    libvlc_media_t *media = libvlc_media_new_path(inst, in_path);
    if (!media) {
        fprintf(stderr, "ERR libvlc_media_new_path failed for %s\n", in_path);
        libvlc_release(inst);
        return 1;
    }
    char sout[4096];
    if (extract) {
        snprintf(sout, sizeof(sout),
                 ":sout=#transcode{vcodec=none,acodec=s16l,channels=2,"
                 "samplerate=44100}:std{access=file,mux=wav,dst=%s}",
                 out_path);
    } else {
        snprintf(sout, sizeof(sout),
                 ":sout=#transcode{acodec=s16l,channels=2,samplerate=44100}"
                 ":std{access=file,mux=wav,dst=%s}",
                 out_path);
    }
    printf("SOUT %s\n", sout + 6); /* skip the leading ":sout=" */
    libvlc_media_add_option(media, sout);
    int rc = play_to_end(inst, media, extract ? "extract" : "transcode");
    libvlc_media_release(media);
    libvlc_release(inst);
    return rc;
}

static int serve(void)
{
    libvlc_instance_t *inst = make_instance();
    if (!inst) return 1;
    libvlc_media_player_t *mp = libvlc_media_player_new(inst);
    if (!mp) {
        fprintf(stderr, "ERR libvlc_media_player_new failed\n");
        libvlc_release(inst);
        return 1;
    }
    printf("OK READY sounddeck-vlc-helper libvlc %s\n", libvlc_get_version());
    fflush(stdout);

    char line[8192];
    while (fgets(line, sizeof(line), stdin)) {
        /* Trim trailing newline/CR. */
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = '\0';
        if (n == 0)
            continue;

        char *sp = strchr(line, ' ');
        char cmd[32];
        const char *arg = "";
        if (sp) {
            size_t clen = (size_t)(sp - line);
            if (clen >= sizeof(cmd)) clen = sizeof(cmd) - 1;
            memcpy(cmd, line, clen);
            cmd[clen] = '\0';
            arg = sp + 1;
        } else {
            snprintf(cmd, sizeof(cmd), "%s", line);
        }

        if (strcasecmp(cmd, "LOAD") == 0) {
            if (!*arg) {
                printf("ERR LOAD requires a path\n");
            } else {
                libvlc_media_t *media = libvlc_media_new_path(inst, arg);
                if (!media) {
                    printf("ERR LOAD libvlc_media_new_path failed\n");
                } else {
                    libvlc_media_player_set_media(mp, media);
                    libvlc_media_release(media);
                    printf("OK LOADED %s\n", arg);
                }
            }
        } else if (strcasecmp(cmd, "PLAY") == 0) {
            if (libvlc_media_player_play(mp) == 0)
                printf("OK PLAYING\n");
            else
                printf("ERR PLAY failed\n");
        } else if (strcasecmp(cmd, "PAUSE") == 0) {
            libvlc_media_player_set_pause(mp, 1);
            printf("OK PAUSED\n");
        } else if (strcasecmp(cmd, "SEEK") == 0) {
            libvlc_time_t ms = (libvlc_time_t)strtoll(arg, NULL, 10);
            libvlc_media_player_set_time(mp, ms);
            printf("OK SEEK %lld\n", (long long)ms);
        } else if (strcasecmp(cmd, "TIME") == 0) {
            libvlc_time_t t = libvlc_media_player_get_time(mp);
            printf("OK TIME %lld %s\n", (long long)t,
                   state_name(libvlc_media_player_get_state(mp)));
        } else if (strcasecmp(cmd, "STOP") == 0) {
            libvlc_media_player_stop(mp);
            printf("OK STOPPED\n");
        } else if (strcasecmp(cmd, "QUIT") == 0) {
            printf("OK BYE\n");
            fflush(stdout);
            break;
        } else {
            printf("ERR unknown command %s\n", cmd);
        }
        fflush(stdout);
    }

    libvlc_media_player_release(mp);
    libvlc_release(inst);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "--version") == 0) {
        printf("%s\n", libvlc_get_version());
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--probe-play") == 0)
        return probe_play(argv[2]);
    if (argc == 4 && strcmp(argv[1], "--probe-transcode") == 0)
        return probe_sout(argv[2], argv[3], 0);
    if (argc == 4 && strcmp(argv[1], "--probe-extract") == 0)
        return probe_sout(argv[2], argv[3], 1);
    if (argc == 2 && strcmp(argv[1], "--serve") == 0)
        return serve();

    fprintf(stderr,
            "usage: %s --version | --probe-play <file> | "
            "--probe-transcode <in> <out.wav> | "
            "--probe-extract <in> <out.wav> | --serve\n",
            argv[0]);
    return 2;
}
