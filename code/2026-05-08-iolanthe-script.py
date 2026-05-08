import re
import subprocess
import sys

# Pass a small number like "10" or "30" to run a quick test.
# Pass a really big number like "1000" to really encode the whole thing.
#
count = int(sys.argv[1])

def to_seconds(ts):
    h, m, s = ts.split(':')
    return int(h) * 3600 + int(m) * 60 + float(s)

segments = []
with open("script.txt", "r") as f:
  for line in f:
    m = re.match(r"^ffmpeg .* -ss (.*) -to (.*) [$]SUFFIX part(.*).mkv", line)
    if m is not None:
      start = to_seconds(m.group(1))
      end = to_seconds(m.group(2))
      idx = m.group(3)
      segments.append((idx, start, end))
segments = [(start, end) for (_, start, end) in sorted(segments)]

segments = segments[:count]

for cc in range(0, len(segments), 20):
  these_segments = segments[cc:cc+20]

  filters = []
  concat_inputs = []
  for i, (start, end) in enumerate(these_segments):
    v = f"[0:v]trim=start={start}:end={end},scale=640:480,setpts=PTS-STARTPTS[v{i}]"
    a = f"[0:a]atrim=start={start}:end={end},asetpts=PTS-STARTPTS[a{i}]"
    filters.append(v)
    filters.append(a)
    concat_inputs.append(f"[v{i}][a{i}]")

  filter_complex = ";".join(filters) + ";"
  filter_complex += "".join(concat_inputs)
  filter_complex += f"concat=n={len(these_segments)}:v=1:a=1[v][a]"

  subprocess.run([
    'ffmpeg', '-y', '-i', 'input.mkv',
    '-filter_complex', filter_complex,
    '-map', '[v]', '-map', '[a]',
    '-c:v', 'libx264', '-crf', '20',
    '-c:a', 'aac', '-b:a', '192k', '-ar', '48000',
    '-movflags', '+faststart',
    f'output-{cc}.mp4'
  ], check=True)

with open("list.txt", "w") as f:
  for cc in range(0, len(segments), 20):
    f.write(f"file 'output-{cc}.mp4'\n")
subprocess.run([
  'ffmpeg', '-y', '-f', 'concat', '-i', 'list.txt',
  '-c', 'copy',
  '-movflags', '+faststart',
  'output.mp4'
], check=True)
