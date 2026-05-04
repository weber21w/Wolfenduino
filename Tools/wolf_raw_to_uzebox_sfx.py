#!/usr/bin/env python3
"""
wolf_raw_to_uzebox_sfx.py

Convert Wolfenstein PC-speaker pitch streams, such as WolfenduinoFX
Assets/RawAudio/audioNN.raw, into Uzebox sound-engine patch command streams.

Input .raw format used by WolfenduinoFX:
	uint32_le length
	uint16_le priority
	uint8_t   pitch_stream[length]
	uint8_t   trailing zero/terminator, usually present but not part of length

Each pitch byte is a 140 Hz PC-speaker pitch tick:
	0 = silence
	N = frequency 1193181 / (N * 60)

Output:
	A readable C .inc file containing one Uzebox patch per audioNN.raw,
	plus a PatchStruct table suitable for InitMusicPlayer().
"""

from __future__ import annotations

import argparse
import math
import os
import re
import struct
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

PC_PIT_HZ = 1193181.0
WOLF_TICK_HZ = 140.0
UZEBOX_TICK_HZ = 60.0

DEFAULT_WAVE = 4
DEFAULT_ENV_VOL = 0xA0
DEFAULT_ENV_SPEED = 0
DEFAULT_PATCH_TABLE_NAME = "WolfSfxPatches"
DEFAULT_PATCH_PREFIX = "wolf_sfx_"


@dataclass
class StepNote:
	index: int
	name: str
	freq: float
	word: int


@dataclass
class RawSound:
	index: int
	filename: str
	length: int
	priority: Optional[int]
	payload: bytes
	trailing_bytes: bytes


@dataclass
class FrameState:
	note: Optional[int]      # None = silence
	freq: float             # averaged source frequency for comments
	raw_desc: str            # source byte/range comment


@dataclass
class Run:
	note: Optional[int]
	frames: int
	freq: float
	raw_desc: str


@dataclass
class Command:
	delay: int
	cmd: str
	value: str
	comment: str = ""


def fail(msg: str) -> None:
	print("error: " + msg, file=sys.stderr)
	sys.exit(1)


def parse_int_auto(text: str) -> int:
	return int(text, 0)


def c_ident(text: str) -> str:
	text = re.sub(r"[^A-Za-z0-9_]", "_", text.strip())
	if not text:
		text = "unnamed"
	if text[0].isdigit():
		text = "_" + text
	return text


def parse_steptable(path: Path) -> List[StepNote]:
	pat = re.compile(
		r"\.word\s+0x([0-9a-fA-F]+)\s*//\s*Note:\s*(\d+)\s*\(([^)]*)\).*?Freq:([0-9.+\-eE]+)"
	)
	notes: List[StepNote] = []
	for line in path.read_text(errors="replace").splitlines():
		m = pat.search(line)
		if not m:
			continue
		word = int(m.group(1), 16)
		idx = int(m.group(2))
		name = m.group(3).strip()
		freq = float(m.group(4))
		notes.append(StepNote(idx, name, freq, word))
	if not notes:
		fail(f"could not parse any notes from steptable: {path}")
	notes.sort(key=lambda n: n.index)
	return notes


def nearest_note(freq: float, notes: Sequence[StepNote], min_note: int, max_note: int) -> int:
	best = None
	best_dist = None
	for n in notes:
		if n.index < min_note or n.index > max_note:
			continue
		d = abs(n.freq - freq)
		if best is None or d < best_dist:  # type: ignore[operator]
			best = n.index
			best_dist = d
	if best is None:
		fail(f"empty note search range {min_note}..{max_note}")
	return best


def note_by_index(notes: Sequence[StepNote]) -> Dict[int, StepNote]:
	return {n.index: n for n in notes}


def wolf_byte_to_freq(v: int) -> float:
	if v == 0:
		return 0.0
	return PC_PIT_HZ / (float(v) * 60.0)


def parse_sounds_h(text: str) -> List[str]:
	# Good enough for Wolf/Sounds.h style enum blocks with comments.
	m = re.search(r"enum\s*\{(?P<body>.*?)\};", text, flags=re.S)
	if not m:
		return []
	body = re.sub(r"/\*.*?\*/", "", m.group("body"), flags=re.S)
	out: List[str] = []
	for line in body.splitlines():
		line = line.split("//", 1)[0]
		parts = [p.strip() for p in line.split(",")]
		for p in parts:
			if not p:
				continue
			p = p.split("=", 1)[0].strip()
			if re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", p):
				out.append(p)
	return out


def load_from_zip(zip_path: Path) -> Tuple[List[Tuple[str, bytes]], Optional[str]]:
	with zipfile.ZipFile(zip_path, "r") as zf:
		names = zf.namelist()
		raw_names = [n for n in names if re.search(r"/Assets/RawAudio/audio\d+\.raw$", n) or re.search(r"(^|/)audio\d+\.raw$", n)]
		# Prefer the canonical WolfenduinoFX path if both generated and copied files exist.
		raw_names.sort(key=lambda n: (0 if "/Assets/RawAudio/" in n else 1, n))
		seen_idx = set()
		raws: List[Tuple[str, bytes]] = []
		for n in raw_names:
			m = re.search(r"audio(\d+)\.raw$", n)
			if not m:
				continue
			idx = int(m.group(1))
			if idx in seen_idx:
				continue
			seen_idx.add(idx)
			raws.append((Path(n).name, zf.read(n)))
		sounds_h = None
		for n in names:
			if n.endswith("Wolf/Sounds.h"):
				sounds_h = zf.read(n).decode("utf-8", errors="replace")
				break
	return raws, sounds_h


def load_from_dir(root: Path) -> Tuple[List[Tuple[str, bytes]], Optional[str]]:
	if (root / "Assets" / "RawAudio").is_dir():
		raw_dir = root / "Assets" / "RawAudio"
	elif root.name == "RawAudio" or list(root.glob("audio*.raw")):
		raw_dir = root
	else:
		fail(f"could not find Assets/RawAudio or audioNN.raw files under {root}")

	raws: List[Tuple[str, bytes]] = []
	for p in sorted(raw_dir.glob("audio*.raw")):
		m = re.match(r"audio(\d+)\.raw$", p.name)
		if m:
			raws.append((p.name, p.read_bytes()))

	sounds_h_path = root / "Wolf" / "Sounds.h"
	sounds_h = sounds_h_path.read_text(errors="replace") if sounds_h_path.exists() else None
	return raws, sounds_h


def raw_sort_key(item: Tuple[str, bytes]) -> int:
	m = re.search(r"audio(\d+)\.raw$", item[0])
	return int(m.group(1)) if m else 999999


def parse_raw_sound(filename: str, data: bytes, skip_header: bool = True) -> RawSound:
	m = re.search(r"audio(\d+)\.raw$", filename)
	idx = int(m.group(1)) if m else 0
	if not skip_header:
		return RawSound(idx, filename, len(data), None, data, b"")

	if len(data) < 7:
		# Too short for the Wolf header, fall back to all bytes.
		return RawSound(idx, filename, len(data), None, data, b"")

	length = struct.unpack_from("<I", data, 0)[0]
	priority = struct.unpack_from("<H", data, 4)[0]
	if length <= len(data) - 6:
		payload = data[6:6 + length]
		trailing = data[6 + length:]
		return RawSound(idx, filename, length, priority, payload, trailing)

	# Bad header; fall back to all bytes rather than silently losing data.
	return RawSound(idx, filename, len(data), None, data, b"")


def frame_source_window(frame: int, src_len: int, src_hz: float, dst_hz: float) -> Tuple[float, float]:
	start = (frame * src_hz) / dst_hz
	end = ((frame + 1) * src_hz) / dst_hz
	if end > src_len:
		end = float(src_len)
	return start, end


def weighted_values_for_frame(data: bytes, frame: int, src_hz: float, dst_hz: float) -> List[Tuple[int, float]]:
	start, end = frame_source_window(frame, len(data), src_hz, dst_hz)
	if end <= start:
		idx = min(len(data) - 1, int(start))
		return [(data[idx], 1.0)]

	weights: Dict[int, float] = {}
	i = int(math.floor(start))
	while i < len(data) and float(i) < end:
		seg_start = max(start, float(i))
		seg_end = min(end, float(i + 1))
		w = max(0.0, seg_end - seg_start)
		if w > 0.0:
			weights[data[i]] = weights.get(data[i], 0.0) + w
		i += 1
	return sorted(weights.items(), key=lambda kv: (-kv[1], kv[0]))


def resample_to_frames(
	data: bytes,
	notes: Sequence[StepNote],
	min_note: int,
	max_note: int,
	src_hz: float,
	dst_hz: float,
	method: str,
	silence_threshold: float,
) -> List[FrameState]:
	if not data:
		return []

	frame_count = max(1, int(math.ceil((len(data) * dst_hz) / src_hz)))
	frames: List[FrameState] = []
	for f in range(frame_count):
		weights = weighted_values_for_frame(data, f, src_hz, dst_hz)
		total_w = sum(w for _, w in weights)
		silence_w = sum(w for v, w in weights if v == 0)
		if total_w <= 0.0 or (silence_w / total_w) >= silence_threshold:
			frames.append(FrameState(None, 0.0, "silence"))
			continue

		nonzero = [(v, w) for v, w in weights if v != 0]
		if method == "mode":
			v = max(nonzero, key=lambda vw: vw[1])[0]
			freq = wolf_byte_to_freq(v)
			raw_desc = f"raw {v}"
		elif method == "nearest":
			center = int(min(len(data) - 1, math.floor(((f + 0.5) * src_hz) / dst_hz)))
			v = data[center]
			if v == 0:
				# Center sample was silent, but frame was not mostly silent. Use strongest nonzero.
				v = max(nonzero, key=lambda vw: vw[1])[0]
			freq = wolf_byte_to_freq(v)
			raw_desc = f"raw {v}"
		else:  # average
			freq_sum = 0.0
			freq_w = 0.0
			min_v = 255
			max_v = 0
			for v, w in nonzero:
				freq_sum += wolf_byte_to_freq(v) * w
				freq_w += w
				min_v = min(min_v, v)
				max_v = max(max_v, v)
			freq = freq_sum / freq_w if freq_w > 0.0 else 0.0
			raw_desc = f"raw {min_v}" if min_v == max_v else f"raw {min_v}..{max_v} avg"

		note = nearest_note(freq, notes, min_note, max_note)
		frames.append(FrameState(note, freq, raw_desc))
	return frames


def rle_frames(frames: Sequence[FrameState]) -> List[Run]:
	if not frames:
		return []
	runs: List[Run] = []
	cur_note = frames[0].note
	cur_frames = 0
	freq_sum = 0.0
	freq_count = 0
	raw_desc = frames[0].raw_desc
	for fr in frames:
		if fr.note != cur_note:
			avg_freq = freq_sum / freq_count if freq_count else 0.0
			runs.append(Run(cur_note, cur_frames, avg_freq, raw_desc))
			cur_note = fr.note
			cur_frames = 0
			freq_sum = 0.0
			freq_count = 0
			raw_desc = fr.raw_desc
		cur_frames += 1
		if fr.freq > 0.0:
			freq_sum += fr.freq
			freq_count += 1
		if fr.raw_desc != raw_desc:
			raw_desc = "var"
	avg_freq = freq_sum / freq_count if freq_count else 0.0
	runs.append(Run(cur_note, cur_frames, avg_freq, raw_desc))
	return runs


def fmt_value(v: int) -> str:
	if v >= 10:
		return str(v)
	return str(v)


def fmt_env_vol(v: int) -> str:
	return "0x%02x" % (v & 0xff)


def emit_with_delay(out: List[Command], delay: int, cmd: str, value: str, comment: str, max_delay: int, pad_cmd: str = "PC_NOTE_UP", pad_value: str = "0") -> None:
	while delay > max_delay:
		out.append(Command(max_delay, pad_cmd, pad_value, "delay pad"))
		delay -= max_delay
	out.append(Command(delay, cmd, value, comment))


def runs_to_commands(
	runs: Sequence[Run],
	note_map: Dict[int, StepNote],
	wave: int,
	env_vol: int,
	env_speed: int,
	max_delay: int,
) -> List[Command]:
	cmds: List[Command] = []
	cmds.append(Command(0, "PC_WAVE", str(wave), "waveform"))
	cmds.append(Command(0, "PC_ENV_SPEED", str(env_speed), "hold volume"))
	cmds.append(Command(0, "PC_ENV_VOL", fmt_env_vol(0), "start muted"))

	cur_volume = 0
	delay = 0
	for run in runs:
		if run.note is None:
			if cur_volume != 0:
				emit_with_delay(cmds, delay, "PC_ENV_VOL", fmt_env_vol(0), "silence", max_delay)
				delay = 0
				cur_volume = 0
			delay += run.frames
			continue

		note = note_map[run.note]
		if cur_volume == 0:
			emit_with_delay(cmds, delay, "PC_ENV_VOL", fmt_env_vol(env_vol), "sound on", max_delay)
			delay = 0
			cur_volume = env_vol

		comment = f"{note.name}, {note.freq:.1f} Hz; source {run.freq:.1f} Hz, {run.raw_desc}"
		emit_with_delay(cmds, delay, "PC_PITCH", str(run.note), comment, max_delay)
		delay = run.frames

	# Wait through the final run, then hard-cut the patch.
	emit_with_delay(cmds, delay, "PC_NOTE_CUT", "0", "end", max_delay)
	cmds.append(Command(0, "PATCH_END", "", ""))
	return cmds


def format_command(c: Command, indent: str = "\t") -> str:
	if c.cmd == "PATCH_END":
		base = f"{indent}{c.delay},{c.cmd}"
	else:
		base = f"{indent}{c.delay},{c.cmd},{c.value}"
	if c.comment:
		return base + "," + "\t" + "/* " + c.comment + " */"
	return base + ","


def make_output(
	raw_sounds: Sequence[RawSound],
	names: Sequence[str],
	notes: Sequence[StepNote],
	args: argparse.Namespace,
) -> str:
	note_map = note_by_index(notes)
	lines: List[str] = []
	lines.append("/*")
	lines.append(" * Generated by wolf_raw_to_uzebox_sfx.py")
	lines.append(" * Source: Wolf PC-speaker 140 Hz pitch streams")
	lines.append(" * Output: Uzebox patch command streams, 60 Hz command deltas")
	lines.append(" *")
	lines.append(" * Include after <uzebox.h> / sound engine definitions so PROGMEM,")
	lines.append(" * PatchStruct, PC_* commands, PATCH_END, and NULL are visible.")
	lines.append(" */")
	lines.append("")
	lines.append(f"#define WOLF_SFX_COUNT {len(raw_sounds)}")
	for snd in raw_sounds:
		name = names[snd.index] if snd.index < len(names) else f"AUDIO{snd.index:02d}_SND"
		lines.append(f"#define WOLF_SFX_{c_ident(name)} {snd.index}")
	lines.append("")

	for snd in raw_sounds:
		name = names[snd.index] if snd.index < len(names) else f"AUDIO{snd.index:02d}_SND"
		ident = args.patch_prefix + c_ident(name)
		frames = resample_to_frames(
			snd.payload,
			notes,
			args.min_note,
			args.max_note,
			args.src_hz,
			args.dst_hz,
			args.method,
			args.silence_threshold,
		)
		runs = rle_frames(frames)
		cmds = runs_to_commands(runs, note_map, args.wave, args.env_vol, args.env_speed, args.max_delay)
		trail = ""
		if snd.trailing_bytes:
			trail = ", trailing=" + " ".join(f"0x{x:02x}" for x in snd.trailing_bytes[:8])
			if len(snd.trailing_bytes) > 8:
				trail += " ..."
		prio = "unknown" if snd.priority is None else str(snd.priority)
		lines.append(f"/* {snd.filename}: {name}, payload={snd.length} ticks @ {args.src_hz:g} Hz, priority={prio}, frames={len(frames)} @ {args.dst_hz:g} Hz{trail} */")
		lines.append(f"const char {ident}[] PROGMEM ={{")
		for c in cmds:
			lines.append(format_command(c))
		lines.append("};")
		lines.append("")

	lines.append(f"const struct PatchStruct {args.patch_table_name}[] PROGMEM = {{")
	for snd in raw_sounds:
		name = names[snd.index] if snd.index < len(names) else f"AUDIO{snd.index:02d}_SND"
		ident = args.patch_prefix + c_ident(name)
		lines.append(f"\t{{0,NULL,{ident},0,0}},\t/* {snd.index:02d}: {name} */")
	lines.append("};")
	lines.append("")
	return "\n".join(lines)


def main(argv: Optional[Sequence[str]] = None) -> int:
	ap = argparse.ArgumentParser(
		description="Convert WolfenduinoFX RawAudio audioNN.raw files to readable Uzebox patch .inc data."
	)
	ap.add_argument("input", type=Path, help="WolfenduinoFX root directory, Assets/RawAudio directory, or .zip file")
	ap.add_argument("--steptable", type=Path, required=True, help="Uzebox steptable.inc")
	ap.add_argument("-o", "--out", type=Path, default=Path("WolfUzeboxSfx.inc"), help="output .inc file")
	ap.add_argument("--sounds-h", type=Path, default=None, help="optional Wolf/Sounds.h for enum names")
	ap.add_argument("--no-skip-header", action="store_true", help="treat whole .raw file as pitch stream, Arduboy-encoder-compatible but usually wrong")
	ap.add_argument("--method", choices=("average", "mode", "nearest"), default="average", help="140 Hz to 60 Hz pitch reduction method")
	ap.add_argument("--silence-threshold", type=float, default=0.50, help="frame is silent if this fraction or more is raw zero")
	ap.add_argument("--src-hz", type=float, default=WOLF_TICK_HZ, help="source pitch tick rate")
	ap.add_argument("--dst-hz", type=float, default=UZEBOX_TICK_HZ, help="Uzebox patch command tick rate")
	ap.add_argument("--wave", type=parse_int_auto, default=DEFAULT_WAVE, help="PC_WAVE value for generated wave patches")
	ap.add_argument("--env-vol", type=parse_int_auto, default=DEFAULT_ENV_VOL, help="PC_ENV_VOL value used when sound is on")
	ap.add_argument("--env-speed", type=parse_int_auto, default=DEFAULT_ENV_SPEED, help="PC_ENV_SPEED value; default 0 holds volume")
	ap.add_argument("--min-note", type=int, default=0, help="lowest Uzebox note index to use")
	ap.add_argument("--max-note", type=int, default=126, help="highest Uzebox note index to use")
	ap.add_argument("--max-delay", type=int, default=120, help="maximum command delta before inserting delay-pad commands")
	ap.add_argument("--patch-prefix", default=DEFAULT_PATCH_PREFIX, help="C symbol prefix for generated patch arrays")
	ap.add_argument("--patch-table-name", default=DEFAULT_PATCH_TABLE_NAME, help="C symbol name for generated PatchStruct table")
	args = ap.parse_args(argv)

	if args.silence_threshold <= 0.0 or args.silence_threshold > 1.0:
		fail("--silence-threshold must be > 0 and <= 1")
	if args.max_delay < 1 or args.max_delay > 255:
		fail("--max-delay must be 1..255")

	notes = parse_steptable(args.steptable)
	if args.min_note < notes[0].index or args.max_note > notes[-1].index or args.min_note > args.max_note:
		fail(f"note range must fit parsed steptable range {notes[0].index}..{notes[-1].index}")

	if args.input.suffix.lower() == ".zip":
		raw_items, sounds_text = load_from_zip(args.input)
	else:
		raw_items, sounds_text = load_from_dir(args.input)
	if not raw_items:
		fail("no audioNN.raw files found")
	raw_items.sort(key=raw_sort_key)

	if args.sounds_h is not None:
		sounds_text = args.sounds_h.read_text(errors="replace")
	names = parse_sounds_h(sounds_text) if sounds_text else []

	raw_sounds = [parse_raw_sound(name, data, skip_header=not args.no_skip_header) for name, data in raw_items]
	out_text = make_output(raw_sounds, names, notes, args)
	args.out.write_text(out_text)

	print(f"wrote {args.out}")
	print(f"sounds: {len(raw_sounds)}")
	if names:
		print(f"names: {len(names)} from Sounds.h")
	else:
		print("names: generic AUDIOxx_SND")
	print(f"method: {args.method}, source {args.src_hz:g} Hz -> Uzebox {args.dst_hz:g} Hz")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
