# {{ lang['name'] }} programming language

This language was in the {{ lang['yearago'] }} position last year, and is now in the {{ lang['now'] }} position. It has a rating of {{ lang['rating'] }}.

It's rating has {%if '-' in lang['change']%}decreased{%else%}increased{%endif%} by {{ lang['change'] }}.

## Top 10 '{{ lang['name'] }}' google searches

Here are 10 google searches for '{{ lang['name'] }}':

{% for search in get_google_results(lang['name']) %}
- {{ search }}{% endfor %}

## Description

{{ askgpt('Explain what the ' + lang["name"] + ' programming language is in a few sentences. Use markdown to format the text.') }}
