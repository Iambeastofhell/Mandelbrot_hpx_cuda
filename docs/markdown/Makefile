PANDOC      := pandoc
PANDOCFLAGS := --from=markdown --to=pdf

ABSTRACT_MD  := abstract.md
ABSTRACT_PDF := $(ABSTRACT_MD:.md=.pdf)

.PHONY: all abstract clean

all: abstract

abstract: $(ABSTRACT_PDF)

$(ABSTRACT_PDF): $(ABSTRACT_MD)
	$(PANDOC) $(PANDOCFLAGS) -o $@ $<

clean:
	rm -f $(ABSTRACT_PDF)

