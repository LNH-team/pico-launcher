SUBSET_MEDIUM = NotoSansJP-Medium-subset.ttf
SUBSET_REGULAR = NotoSansJP-Regular-subset.ttf
TARGETS_MEDIUM = NotoSansJP-Medium-10.nft2 NotoSansJP-Medium-11.nft2
TARGETS_REGULAR = NotoSansJP-Regular-10.nft2

all: $(TARGETS_MEDIUM) $(TARGETS_REGULAR)

chars_translate.txt: gen_transchars.py
	@echo "  GEN $@"
	@python3 gen_transchars.py . $@

$(SUBSET_MEDIUM): NotoSansJP-Medium.ttf chars_preset.txt chars_translate.txt
	@echo "  SUBSET $@"
	@cat chars_preset.txt chars_translate.txt > chars_all.txt
	@python3 -m fontTools.subset NotoSansJP-Medium.ttf --text-file=chars_all.txt --output-file=$@
	@rm -f chars_all.txt

$(SUBSET_REGULAR): NotoSansJP-Regular.ttf chars_preset.txt chars_translate.txt
	@echo "  SUBSET $@"
	@cat chars_preset.txt chars_translate.txt > chars_all.txt
	@python3 -m fontTools.subset NotoSansJP-Regular.ttf --text-file=chars_all.txt --output-file=$@
	@rm -f chars_all.txt

$(TARGETS_MEDIUM): $(SUBSET_MEDIUM)
	@echo "  GEN $@"
	@python3 ./gen_nft2.py $^ 10 NotoSansJP-Medium-10.nft2
	@python3 ./gen_nft2.py $^ 11 NotoSansJP-Medium-11.nft2

$(TARGETS_REGULAR): $(SUBSET_REGULAR)
	@echo "  GEN $@"
	@python3 ./gen_nft2.py $^ 10 $@

override: $(TARGETS_MEDIUM) $(TARGETS_REGULAR)
	cp $(TARGETS_MEDIUM) $(TARGETS_REGULAR) ../data/
