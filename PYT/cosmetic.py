"""
ciekawe statystyki z jsona
wczytuj z argparsem --input i --output

porownać czas dzialania programu na pythonie 3.12 i <3.12
"""

import argparse
from pathlib import Path
import pandas as pd
from sys import stdout
from json import dump

parser = argparse.ArgumentParser()
parser.add_argument('--input', type=Path, required=True)
parser.add_argument('--output', type=Path, required=False) # stdout jesli nie podano
parser.add_argument('--head', type=int, required=False, help='ile max wierszy wypisac', default=None)

args = parser.parse_args()

if __name__ != '__main__':
    exit(1)

output_data = {}

def add_df_to_json(key: str, df: pd.DataFrame):
    global output_data
    output_data[key] = df.head(args.head).to_dict()
    
   
if not args.input or not args.input.exists():
    raise FileNotFoundError('Input file does not exist')

df = pd.read_json(args.input)

add_df_to_json(
    "srednia cena najczesciej wystepujacych produktow wg. waluty",
    df
        .groupby(['product_type', 'currency'])['price']
        .aggregate(['count', 'mean'])
        .sort_values(by='count', ascending=False)
        .reset_index()
        .rename(columns={'mean': 'avg_price', 'count': 'count'})
        
)

product_count = df.count()['product_type']

add_df_to_json(
    "najczesciej wystepujace tagi ich ilosc i procent",
    df
        .explode('tag_list')
        .value_counts('tag_list')
        .to_frame()
        .rename(columns={0: 'count'})
        .assign(percentage=lambda df: df['count'] * 100 / product_count)
)

add_df_to_json(
    "srednia cena dla kazdej firmy",
    df
        .groupby(['brand', 'currency'])['price']
        .aggregate(['mean'])
        .sort_values(by='mean', ascending=False)
        .reset_index()
        .rename(columns={'mean': 'avg_price'})
)

add_df_to_json(
    "kolory i ile razy wystepuja",
    df
        .explode('product_colors')
        ['product_colors']
        .map(lambda x: x["hex_value"].upper() if isinstance(x, dict) else None)
        .dropna()
        .map(lambda x: x if len(x) == 7 and x[0] == "#" else None) # ktos zle powpisywal kolory hex
        .dropna()
        .to_frame()
        .value_counts()
        .dropna()
        .reset_index()
)

add_df_to_json(
    "najczestsze TLD w stronach",
    df
        .explode('website_link')
        ['website_link']
        .map(lambda x: x.split('.')[-1].split('/')[0])
        .to_frame()
        .value_counts()
        .reset_index()
        .rename(columns={0: 'count'})
)

if args.output:
    if args.output.suffix != '.json':
        raise ValueError('Output file must be a json file')
    with open(args.output, 'w+') as f:
        dump(output_data, f)
else:
    dump(output_data, stdout, indent=4)