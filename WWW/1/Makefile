md-out: make.py *.j2.md
	python3 make.py

html: md-out
	mkdir -p html
	for f in md-out/*.md; do \
		pandoc -s -o html/`basename $$f .md`.html $$f --metadata pagetitle=`basename $$f .md` --template=template.html; \
	done

clean:
	rm -rf md-out html

install: html
	mkdir -p ~/public_html/www/1
	cp -r html/* ~/public_html/www/1
	chmod -R a+rX ~/public_html/www/1

.PHONY: clean