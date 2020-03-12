#! /usr/bin/python3

import os

PROC_FREQ=float(os.environ.get('PROC_FREQ', '2.0')) # GHz

RES_FILE=os.environ.get('RES_FILE', '../spook_bench/results.txt')

def parse_spook_id(s):
    clyde_f, shadow_f, *arch = s.split('-')[:-1]
    arch = '-'.join(arch)
    clyde = clyde_f.split('_')[0]
    shadow = shadow_f.split('_')[0]
    return ((clyde, shadow), '-'.join((clyde_f, shadow_f)), arch)

def fmt_cycles(cycles):
    try:
        return '{:.2f}'.format(cycles)
    except ValueError:
        return cycles

def parse_line(s):
    try:
        spook_id, res = s.split('|')
    except ValueError:
        # Missing value due to failed benchmark
        spook_id = s.strip()
        val = None
    else:
        n_bytes, _, _, ns_iter, _, ns_byte = res.strip().split(' ')
        throughput = PROC_FREQ*float(ns_byte)
        val = (int(n_bytes), throughput)
    _, implem, arch = parse_spook_id(spook_id.strip())
    return ((implem, arch), val)

def render_markdown_table(headers, rows, data):
    full_table = [[' ']+headers] + [['-' for _ in ['']+headers]] + [[r]+d for r, d in zip(rows, data)]
    return '\n'.join('|'+'|'.join(row)+'|' for row in full_table)


results = dict()
for (implem, arch), val in map(parse_line, open(RES_FILE).read().splitlines()):
    if val is not None:
        results.setdefault((implem, arch), dict())[val[0]] = val[1]

print('results', results)

implems = list(sorted(set(implem for implem, _ in results.keys())))

archs = ['x86-64', 'haswell', 'skylake-avx512']

max_throughput_table = [
        [fmt_cycles(min(results.get((implem, arch), dict()).values(), default=' ')) for arch in archs]
        for implem in implems]

def throughput_bytes(n):
    return [[fmt_cycles(results.get((implem, arch), dict()).get(n, ' ')) for arch in archs] for implem in implems]

print(
        'max throughput (cycles/byte):\n\n',
        render_markdown_table(archs, implems, max_throughput_table), '\n'
        )

n=2048
print('throughput (cycles/byte) for n={} bytes:\n\n'.format(n), render_markdown_table(archs, implems, throughput_bytes(n)))
