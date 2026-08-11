# Overview

This enables language selection for menu and label displays in pico-launcher.

The GUI implementation and translation implementation are separated, similar to multilingual support using GNU gettext.

# GUI Implementation Procedure

- Work in the `arm9/source` directory.
- When specifying strings during GUI implementation, you need to insert the function `_()` as an argument to `SetText()`. This is not necessary if multilingual display is not required.
- Run make in the project's root directory as usual.
- If new strings requiring translation are added, a difference will appear in `arm9/locale/message.pot`. Existing translation files `arm9/locale/*.po` will also be updated simultaneously.
- Commit/push the GUI implementation, `arm9/locale/message.pot`, and `arm9/locale/*.po`.

# Translation Procedure

In many cases, font updates are necessary to verify the translation results.

- Work in the `arm9/locale` directory.
- If a .po file for the target language does not exist, execute the following. `<language>` must be a string with a language code that gettext understands (e.g., en_GB, ja, etc.).
  - `msginit --no-translator -i messages.pot -l <language> -o <language>.po`
- In the `msgstr` line inside the .po file, write the translation result of the string specified in the corresponding `msgid`.
- Also, ensure that the `charset` in the `Content-Type` line inside the .po file is `UTF-8`.
- Commit/push the .po file.

# Font Update Procedure

Currently, the only font used to generate the nft2 files is NotoSansJP, so characters not included in this font will not render.

- Work in the `arm9/locale` directory.
- Obtain the NotoSansJP font and place it in the same directory.
- Run `make` in this directory. This will generate the `.nft2` files.
- If `chars_translate.txt` has no differences from the previous version, the following steps are unnecessary.
- Run `make override` to overwrite the `.nft2` files used for the build.
- Run `make` in the project's root directory.
- Test the pico-launcher and confirm that the translated string is displayed.
- After confirmation, commit/push both chars_translate.txt and the updated `.nft2` files under `arm9/data`.

To support all languages and writing systems, it would be necessary to address two separate requirements:

- merging the contents of `.nft2` files generated from other fonts containing missing glyphs into a single file, and
- extending the `.nft2` format to include different glyph variants for the same code point.

However, a practical and easy-to-maintain solution for these challenges has not yet been established.
