PDFLATEX=pdflatex
MAIN=main

all:
	$(PDFLATEX) $(MAIN).tex

watch:
	latexmk -pdf -pvc $(MAIN).tex

clean:
	latexmk -c

distclean: clean
	rm -f $(MAIN).pdf
