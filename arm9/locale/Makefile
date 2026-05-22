chars_translate.txt: gen_transchars.py ../../build/arm9/arm9/locale/translate.c
	@echo "  GEN $@"
	@python3 gen_transchars.py . $@

ttfsubset: chars_preset.txt chars_translate.txt
	@echo "  SUBSET $@"
	@cat $^ > chars_all.txt
	@python3 -m fontTools.subset NotoSansJP-Medium.ttf --text-file=chars_all.txt --output-file=NotoSansJP-Medium-subset.ttf
	@python3 -m fontTools.subset NotoSansJP-Regular.ttf --text-file=chars_all.txt --output-file=NotoSansJP-Regular-subset.ttf
	@rm -f chars_all.txt
