
DOC = functracer_internals
FIGS = functracer_architecture.eps

all: $(DOC).pdf $(DOC).html

$(DOC).pdf: $(DOC).txt
	TMPDIR=$(shell mktemp -d) && \
	asciidoc -d article -b docbook -o $$TMPDIR/file.xml $< && \
	cp $(FIGS) $$TMPDIR/ && \
	dblatex --pdf -o $@ $$TMPDIR/file.xml; \
	#(cd $$TMPDIR && docbook2pdf -o . file.xml) && \
	#mv $$TMPDIR/file.pdf $@; \
	rm -rf $$TMPDIR

$(DOC).html: $(DOC).txt
	asciidoc -o $@ $<

clean:
	rm -f $(DOC).pdf $(DOC).html
