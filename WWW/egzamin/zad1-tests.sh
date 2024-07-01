#!/bin/bash

printf "Test 1\n"
curl -X 'GET' \
	'http://localhost:8000/sentence_count?seed=42' \
	-H 'accept: application/json'

printf "\nTest 2\n"
curl -X 'GET' \
	'http://localhost:8000/link_count?seed=42' \
	-H 'accept: application/json'

printf "\nTest 3\n"
curl -X 'POST' \
	'http://localhost:8000/all_pages_and_sentences' \
	-H 'accept: application/json' \
	-H 'Content-Type: application/json' \
	-d '{
  "seeds": [
    0,1,2,3
  ]
}'

printf "\nTest 4\n"
curl -X 'GET' \
	'http://localhost:8000/all_pages_and_sentences_k?seed=0&k=3' \
	-H 'accept: application/json'

printf "\n"
