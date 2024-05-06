# Top 20 programming languages

Here's a list of the top 20 programming languages:

{% for lang in top20 %}

### {{ lang['name'] }}

[link to {{ lang['name'] }}'s page](./{{ make_slug(lang['name']) }}.html)

Position last year: {{ lang['yearago'] }}
Position now: {{ lang['now'] }}
Rating: {{ lang['rating'] }}

---

{% endfor %}