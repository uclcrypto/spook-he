#! /usr/bin/python3

import os

PROC_FREQ=float(os.environ.get('PROC_FREQ', '2.0')) # GHz
PRIM_NS=6 # nb of steps in clyde/shadow (i.e. nmb of iterations of the iaca loop)

IACA_RES_FILE=os.environ.get('IACA_RES_FILE', '../prim_bench_iaca/results.txt')
REAL_RES_FILE=os.environ.get('REAL_RES_FILE', '../prim_bench_real/results.txt')

def parse_prim_id(s):
    prim_implem, *arch = s.split('-')[:-1]
    arch = '-'.join(arch)
    prim = prim_implem.split('_')[0]
    return (prim, prim_implem, arch)

def fmt_cycles(cycles):
    return '{:.2f}'.format(cycles)

def parse_real_line(s):
    try:
        prim_id, ns_iter = s.split(' ')
        val = fmt_cycles(PROC_FREQ*float(ns_iter))
    except ValueError:
        # Missing value due to failed benchmark
        prim_id = s.strip()
        val = ' '
    return (parse_prim_id(prim_id), val)

def parse_iaca_line(s):
    print(s)
    prim_id, cycles = s.split(' ')[:2]
    cycles = PRIM_NS*float(cycles)
    return (parse_prim_id(prim_id), fmt_cycles(cycles))

def render_markdown_table(headers, rows, data):
    full_table = [[' ']+headers] + [['-' for _ in ['']+headers]] + [[r]+d for r, d in zip(rows, data)]
    return '\n'.join('|'+'|'.join(row)+'|' for row in full_table)


iaca_results = dict(map(parse_iaca_line,
    open(IACA_RES_FILE).read().splitlines()))
real_results = dict(map(parse_real_line,
    open(REAL_RES_FILE).read().splitlines()))

clyde_implems = list(sorted(set(x[1] for x in real_results.keys() if x[0] == 'clyde')))
shadow_implems = list(sorted(set(x[1] for x in real_results.keys() if x[0] == 'shadow')))

archs = ['x86-64', 'haswell', 'skylake-avx512']

clyde_iaca_table = [
        [iaca_results.get(('clyde', implem, arch), ' ') for arch in archs]
        for implem in clyde_implems]

shadow_iaca_table = [
        [iaca_results.get(('shadow', implem, arch), ' ') for arch in archs]
        for implem in shadow_implems]

clyde_real_table = [
        [real_results.get(('clyde', implem, arch), ' ') for arch in archs]
        for implem in clyde_implems]

shadow_real_table = [
        [real_results.get(('shadow', implem, arch), ' ') for arch in archs]
        for implem in shadow_implems]

print('clyde iaca:\n\n',render_markdown_table(archs, clyde_implems, clyde_iaca_table), '\n')
print('clyde real:\n\n',render_markdown_table(archs, clyde_implems, clyde_real_table), '\n')
print('shadow iaca:\n\n',render_markdown_table(archs, shadow_implems, shadow_iaca_table), '\n')
print('shadow real:\n\n',render_markdown_table(archs, shadow_implems, shadow_real_table), '\n')
