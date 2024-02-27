from bs4 import BeautifulSoup
import requests
from googlesearch import search
from jinja2 import Template, Environment
from g4f.client import Client
import os
import re

DIRECTORY = 'md-out'

def fetch_top20():
    r = requests.get('https://www.tiobe.com/tiobe-index/')
    r.raise_for_status()
    soup = BeautifulSoup(r.text, 'html.parser')
    data = []
    for row in soup.select('#top20 tr') or []:
        if tds := row.select('td'):
            yearago, now, _, _, lang, ratings, change = tds
            data.append({
                'yearago': yearago.text,
                'now': now.text,
                'change': change.text,
                'name': lang.text,
                'rating': ratings.text
            })
    return data

gpt = Client()

def askgpt(prompt: str):
    try:
        resp = gpt.chat.completions.create(
            model="gpt-3.5-turbo",
            messages=[
                {"role": "system", "content": "You are a website designer. Return responses in Markdown format."},
                {"role": "user", "content": prompt},
            ],
        )
        return resp.choices[0].message.content
    except Exception as e:
        return f"<!-- Couldn't fetch response from GPT-3.5, error: {e} -->"
    

def get_google_results(lang: str):
    try:
        return list(search(f'{lang} programming language', num=10, stop=10))
    except Exception as e:
        return []

def make_slug(t):
    return re.sub(r'[^a-z0-9+\-]', '_', t.lower())

if os.path.exists(DIRECTORY):
    os.system(f'rm -rf {DIRECTORY}')
os.mkdir(DIRECTORY)

env = Environment()
data = {
    'top20': fetch_top20()[:5],
    'date': os.popen('date').read(),
    'get_google_results': get_google_results,
    'make_slug': make_slug,
    'askgpt': askgpt
}

with open(f'{DIRECTORY}/index.md', 'w') as f:
    print("Writing index.md")
    template = env.from_string(open('index.j2.md').read())
    f.write(template.render(**data))

for lang in data['top20']:
    print(f"Writing {DIRECTORY}/{make_slug(lang['name'])}.md")
    with open(f'{DIRECTORY}/{make_slug(lang["name"])}.md', 'w') as f:
        template = env.from_string(open('[lang].j2.md').read())
        f.write(template.render(lang=lang, **data))