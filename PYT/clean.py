from functools import reduce
from sys import argv
import json

if len(argv) < 2 or not argv[1].endswith('.ipynb'):
    print(f'Usage: {argv[0]} <filename>.ipynb')
    exit(1)

with open(argv[1], 'r', encoding='utf-8') as f:
    data = json.load(f)
    
data['cells'] = list(map(
    lambda cell: dict(filter(
        lambda x: x[0] != 'outputs',
        cell.items()
    )),
    data.get('cells', [])
))

def remove_code(prev_cells, cell):
    if len(prev_cells) == 0:
        return [cell]
    
    if cell.get('cell_type', '') != 'code':
        return prev_cells + [cell]
    
    if prev_cells[-1].get('cell_type', '') != 'markdown':
        return prev_cells + [cell]
    
    if "# Ćwiczenie" in (prev_cells[-1].get('source', '') or [""])[0]:
        return prev_cells
    
    return prev_cells + [cell]
    

data['cells'] = reduce(remove_code, data['cells'], [])

with open(argv[1][:-len('.ipynb')] + '.clean.ipynb', 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=2, ensure_ascii=False)