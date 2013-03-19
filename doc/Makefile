DOC=writeup

all: $(DOC).pdf

$(DOC).pdf: $(DOC).tex
	pdflatex -file-line-error -interaction=nonstopmode $(DOC) 
	pdflatex -file-line-error -interaction=nonstopmode $(DOC) 

clean:
	rm $(DOC).aux $(DOC).log $(DOC).out $(DOC).pdf
