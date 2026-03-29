# Hide Patterns
An optional list of file and folder names to hide from the browser.
Each entry is a literal name or a glob pattern using `*` (matches any characters) and `?` (matches exactly one character). Matching is case-insensitive. Up to 8 patterns at a time are supported.

## How to setup hide patterns
1. Open `/_pico/settings.json`.
2. If not present yet, add a `hidePatterns` key.
3. For a folder or file that you wish to hide from the browser add an entry inside the `hidePatterns` key. For example:
    ```json
    "hidePatterns": [
        "_pico",
        "_picoboot.nds"
    ]
    ```
    (if you are using DSpico)

Or to simply hide everything that starts with an underscore:

```json
"hidePatterns": [
    "_*"
]
```

> **Note**  
> Using a `*` without any characters next to it will hide **everything** in the browser